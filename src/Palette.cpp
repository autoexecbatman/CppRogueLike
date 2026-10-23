// file: Palette.cpp
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "Palette.h"
#include "Paths.h"
#include "TileRefJson.h"

using json = nlohmann::json;

namespace
{
// Whether `symbol` is one of the five structural symbols, which no entry may take.
//
// Example:
//   is_structural('#');   // -> true
//   is_structural('W');   // -> false
[[nodiscard]] bool is_structural(char symbol)
{
	const auto has_symbol = [symbol](const StructuralSymbol& structural)
	{
		return structural.symbol == symbol;
	};
	return std::ranges::any_of(STRUCTURAL_SYMBOLS, has_symbol);
}

// The index of the entry carrying `symbol`, if one does; a symbol of 0 is carried by none.
[[nodiscard]] std::optional<size_t> find_symbol(
	const std::vector<PaletteEntry>& entries,
	char symbol)
{
	if (symbol == 0)
	{
		return std::nullopt;
	}
	for (size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
	{
		if (entries[entryIndex].symbol == symbol)
		{
			return entryIndex;
		}
	}
	return std::nullopt;
}

// The index of the entry showing `tile`, if one does.
[[nodiscard]] std::optional<size_t> find_tile(
	const std::vector<PaletteEntry>& entries,
	TileRef tile)
{
	for (size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
	{
		if (entries[entryIndex].tile == tile)
		{
			return entryIndex;
		}
	}
	return std::nullopt;
}

// Whether giving the entry at `ownIndex` this label and symbol would break an invariant,
// and which; `ownIndex` is empty for an entry not yet in the palette.
//
// Example:
//   edit_refusal(entries, std::nullopt, "Barrel", 'W');   // -> DUPLICATE_SYMBOL when an entry has 'W'
//   edit_refusal(entries, 0, "Cobweb", 'W');              // -> APPLIED: entry 0 keeps its own 'W'
[[nodiscard]] PaletteEdit edit_refusal(
	const std::vector<PaletteEntry>& entries,
	std::optional<size_t> ownIndex,
	std::string_view label,
	char symbol)
{
	if (label.empty())
	{
		return PaletteEdit::EMPTY_LABEL;
	}
	if (is_structural(symbol))
	{
		return PaletteEdit::STRUCTURAL_SYMBOL;
	}
	const std::optional<size_t> holder = find_symbol(entries, symbol);
	if (holder && holder != ownIndex)
	{
		return PaletteEdit::DUPLICATE_SYMBOL;
	}
	return PaletteEdit::APPLIED;
}
} // namespace

void to_json(json& output, const PaletteEntry& entry)
{
	output = TileRefJson::encode_tile(entry.tile);
	output["label"] = entry.label;
	output["symbol"] =
		entry.symbol ? std::string(1, entry.symbol) : std::string{};
}

void from_json(const json& source, PaletteEntry& entry)
{
	entry.tile = TileRefJson::parse_tile(source);
	entry.label = source.at("label").get<std::string>();
	const std::string symbolString = source.at("symbol").get<std::string>();
	entry.symbol = symbolString.empty() ? 0 : symbolString[0];
}

std::string_view palette_edit_refusal(PaletteEdit edit)
{
	switch (edit)
	{
	case PaletteEdit::APPLIED:
	{
		assert(
			false && "palette_edit_refusal: an applied edit is not a refusal");
		return "";
	}

	case PaletteEdit::DUPLICATE_SYMBOL:
	{
		return "another entry already has that symbol";
	}

	case PaletteEdit::DUPLICATE_TILE:
	{
		return "another entry already shows that tile";
	}

	case PaletteEdit::STRUCTURAL_SYMBOL:
	{
		return "# . , + ~ are fixed for walls, floor, corridor, doors and water";
	}

	case PaletteEdit::UNKNOWN_SYMBOL:
	{
		return "no entry has that symbol";
	}

	case PaletteEdit::EMPTY_LABEL:
	{
		return "a label cannot be empty";
	}
	}

	std::unreachable();
}

void Palette::load(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream paletteFile(resolved);
	if (!paletteFile.is_open())
	{
		throw std::runtime_error(
			std::format(
				"Palette::load -- cannot open '{}' -- the palette is required",
				resolved.string()));
	}

	// Read into a local, so a refused file leaves the palette that was loaded before it.
	const json document = json::parse(paletteFile);
	const std::vector<PaletteEntry> loaded =
		document.at("palette").get<std::vector<PaletteEntry>>();

	// Each entry is checked against the ones before it, so a refusal names the later of two clashing entries.
	std::vector<PaletteEntry> accepted;
	for (const PaletteEntry& entry : loaded)
	{
		PaletteEdit refusal =
			edit_refusal(accepted, std::nullopt, entry.label, entry.symbol);
		if (refusal == PaletteEdit::APPLIED && find_tile(accepted, entry.tile))
		{
			refusal = PaletteEdit::DUPLICATE_TILE;
		}
		if (refusal != PaletteEdit::APPLIED)
		{
			std::string shownLabel = entry.label;
			if (shownLabel.empty())
			{
				shownLabel = "<empty label>";
			}
			throw std::runtime_error(
				std::format(
					"Palette::load -- '{}' entry '{}' (sheet {} col {} row {}) is refused: {}",
					resolved.string(),
					shownLabel,
					static_cast<int>(entry.tile.sheet),
					entry.tile.col,
					entry.tile.row,
					palette_edit_refusal(refusal)));
		}
		accepted.push_back(entry);
	}

	paletteEntries = std::move(accepted);
	sourcePath = std::string(path);
	std::clog << std::format("[Palette] loaded: {}\n", resolved.string());
}

void Palette::save() const
{
	assert(
		!sourcePath.empty() &&
		"Palette::save: called before a load named the file");

	const auto resolved = Paths::resolve(sourcePath);

	// The rest of the file belongs to TileConfig, so a file that cannot be read back whole is not written.
	std::ifstream existingFile(resolved);
	if (!existingFile.is_open())
	{
		throw std::runtime_error(
			std::format(
				"Palette::save -- cannot open '{}' to keep its other sections",
				resolved.string()));
	}
	json document;
	try
	{
		document = json::parse(existingFile);
	}
	catch (const json::parse_error& error)
	{
		throw std::runtime_error(
			std::format(
				"Palette::save -- '{}' does not parse: {}",
				resolved.string(),
				error.what()));
	}
	existingFile.close();

	document["palette"] = paletteEntries;

	std::ofstream out(resolved);
	if (!out.is_open())
	{
		throw std::runtime_error(
			std::format(
				"Palette::save -- cannot open '{}' for writing",
				resolved.string()));
	}
	out << document.dump(4);
	out.flush();
	if (out.fail())
	{
		throw std::runtime_error(
			std::format(
				"Palette::save -- write failed for '{}'",
				resolved.string()));
	}
	std::clog << std::format("[Palette] saved: {}\n", resolved.string());
}

PaletteEdit Palette::add_or_update(
	TileRef tile,
	std::string_view label,
	char symbol)
{
	assert(tile.is_valid() && "Palette::add_or_update: an entry needs a tile");

	const std::optional<size_t> ownIndex = find(tile);
	const PaletteEdit refusal =
		edit_refusal(paletteEntries, ownIndex, label, symbol);
	if (refusal != PaletteEdit::APPLIED)
	{
		return refusal;
	}

	if (ownIndex)
	{
		paletteEntries[*ownIndex].label = label;
		paletteEntries[*ownIndex].symbol = symbol;
	}
	else
	{
		paletteEntries.push_back(
			PaletteEntry{ tile, std::string(label), symbol });
	}
	return PaletteEdit::APPLIED;
}

void Palette::remove(TileRef tile)
{
	const auto shows_tile = [tile](const PaletteEntry& entry)
	{
		return entry.tile == tile;
	};
	std::erase_if(paletteEntries, shows_tile);
}

std::optional<size_t> Palette::find(TileRef tile) const
{
	return find_tile(paletteEntries, tile);
}

PaletteEdit Palette::set_symbol_tile(char symbol, TileRef tile)
{
	assert(
		tile.is_valid() && "Palette::set_symbol_tile: an entry needs a tile");

	if (is_structural(symbol))
	{
		return PaletteEdit::STRUCTURAL_SYMBOL;
	}
	const std::optional<size_t> ownIndex = find_symbol(paletteEntries, symbol);
	if (!ownIndex)
	{
		return PaletteEdit::UNKNOWN_SYMBOL;
	}
	const std::optional<size_t> holder = find(tile);
	if (holder && holder != ownIndex)
	{
		return PaletteEdit::DUPLICATE_TILE;
	}

	paletteEntries[*ownIndex].tile = tile;
	return PaletteEdit::APPLIED;
}

PaletteEdit Palette::set_symbol_label(char symbol, std::string_view label)
{
	if (label.empty())
	{
		return PaletteEdit::EMPTY_LABEL;
	}
	if (is_structural(symbol))
	{
		return PaletteEdit::STRUCTURAL_SYMBOL;
	}
	const std::optional<size_t> ownIndex = find_symbol(paletteEntries, symbol);
	if (!ownIndex)
	{
		return PaletteEdit::UNKNOWN_SYMBOL;
	}

	paletteEntries[*ownIndex].label = label;
	return PaletteEdit::APPLIED;
}

std::vector<SymbolRow> Palette::symbol_rows() const
{
	std::vector<SymbolRow> rows;
	for (const StructuralSymbol& structural : STRUCTURAL_SYMBOLS)
	{
		rows.push_back(
			SymbolRow{ structural.symbol, std::string(structural.label) });
	}
	for (const PaletteEntry& entry : paletteEntries)
	{
		if (entry.symbol != 0)
		{
			rows.push_back(SymbolRow{ entry.symbol, entry.label });
		}
	}
	return rows;
}

TileRef Palette::resolve_decor(char symbol) const
{
	const std::optional<size_t> holder = find_symbol(paletteEntries, symbol);
	return holder ? paletteEntries[*holder].tile : TileRef{};
}

std::string Palette::symbol_label(char symbol) const
{
	for (const StructuralSymbol& structural : STRUCTURAL_SYMBOLS)
	{
		if (structural.symbol == symbol)
		{
			return std::string(structural.label);
		}
	}
	const std::optional<size_t> holder = find_symbol(paletteEntries, symbol);
	return holder ? paletteEntries[*holder].label : std::string(1, symbol);
}
