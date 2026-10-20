// file: Palette.h
//
// The decoration palette: every tile a designer has named, with the prefab symbol that
// stands for it. One Palette, owned by Game and reached through ctx.palette, is the only
// copy - the decoration editor edits it by tile, the room editor by symbol, and prefab
// stamping reads it. It is loaded from and saved to the "palette" section of
// tile_config.json; the rest of that file belongs to TileConfig and is kept on save.
//
// The palette holds four invariants, and every edit that would break one is refused and
// changes nothing: every label is non-empty, every tile appears once, every symbol
// appears once, and no entry takes one of the five structural symbols (# . , + ~),
// which prefabs use for walls, floor, corridor, doors and water.
//
// Example:
//   Palette palette;
//   palette.load(Paths::TILE_CONFIG);                       // the shipped palette; save() writes back there
//   palette.symbol_label('W');                              // -> "Cobweb"
//   palette.set_symbol_label('W', "Spider web");            // -> PaletteEdit::APPLIED
//   palette.set_symbol_label('#', "Stone");                 // -> PaletteEdit::STRUCTURAL_SYMBOL, nothing changed
//   palette.save();                                         // the file's palette section now says "Spider web"
//   palette.resolve_decor('W');                             // -> the cobweb tile, sheet 38 col 0 row 2
//   palette.resolve_decor('#');                             // -> TileRef{}, which is_valid() calls false
#pragma once

#include <array>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Renderer.h"

// One named tile. A symbol of 0 means the tile has no prefab character.
struct PaletteEntry
{
	TileRef tile{};
	std::string label{};
	char symbol{ 0 };

	[[nodiscard]] bool operator==(const PaletteEntry& other) const = default;
};

// nlohmann's readers and writers for a palette row: the tile's three fields, "label" and
// "symbol", where an empty symbol string is a symbol of 0. Every field is required.
void to_json(nlohmann::json& output, const PaletteEntry& entry);
void from_json(const nlohmann::json& source, PaletteEntry& entry);

// A symbol a prefab can use and the name it is shown under, structural or not.
struct SymbolRow
{
	char symbol{ 0 };
	std::string label{};
};

// A symbol every prefab understands without a palette entry; it carries no sprite.
struct StructuralSymbol
{
	char symbol{ 0 };
	std::string_view label{};
};

inline constexpr std::array<StructuralSymbol, 5> STRUCTURAL_SYMBOLS{ {
	{ '#', "Wall" },
	{ '.', "Floor" },
	{ ',', "Corridor" },
	{ '+', "Door" },
	{ '~', "Water" },
} };

// What an edit did: APPLIED, or the invariant it would have broken, in which case the
// palette is unchanged.
enum class PaletteEdit
{
	APPLIED,
	DUPLICATE_SYMBOL,
	DUPLICATE_TILE,
	STRUCTURAL_SYMBOL,
	UNKNOWN_SYMBOL,
	EMPTY_LABEL,
};

// The sentence an editor shows for a refused edit.
//
// Example:
//   palette_edit_refusal(PaletteEdit::DUPLICATE_SYMBOL);   // -> "another entry already has that symbol"
//   palette_edit_refusal(PaletteEdit::APPLIED);            // aborts: an applied edit is not a refusal
[[nodiscard]] std::string_view palette_edit_refusal(PaletteEdit edit);

class Palette
{
private:
	std::vector<PaletteEntry> paletteEntries{};
	std::string sourcePath{}; // the file the last successful load read, where save writes

public:
	// Reads the "palette" section of the file at `path` and remembers the path for save().
	// Every entry needs sheet, col, row, label and symbol. Throws std::runtime_error when
	// the file does not open, and a refusal naming the entry when one breaks an invariant;
	// nlohmann's errors pass through for a file that does not parse, a missing "palette"
	// or a missing field. A refused load keeps the palette and path loaded before it.
	//
	// Example:
	//   palette.load(Paths::TILE_CONFIG);    // 32 entries, 17 of them with a symbol
	//   palette.load("data/missing.json");   // throws: "Palette::load -- cannot open '...missing.json' -- the palette is required"
	void load(std::string_view path);

	// Writes the palette into the "palette" section of the file load() read, keeping every
	// other section. Throws std::runtime_error naming the path when the file does not open
	// or parse, which would lose those sections, and leaves it untouched; also when the
	// write fails, which may leave the file partly written. Calling it before a load is a
	// wiring fault and asserts.
	//
	// Example:
	//   palette.save();   // the file's palette section now holds this palette
	//   palette.save();   // file made unparseable meanwhile: throws "Palette::save -- '...' does not parse: ..."
	void save() const;

	// Gives `tile` this label and symbol, adding it when the palette does not hold it. A
	// symbol of 0 means none. `tile` must be valid.
	//
	// Example:
	//   palette.add_or_update(cobweb, "Old web", 'W');     // -> APPLIED, the cobweb renamed
	//   palette.add_or_update(barrel, "Barrel", 'W');      // -> DUPLICATE_SYMBOL, the cobweb has 'W'
	[[nodiscard]] PaletteEdit add_or_update(
		TileRef tile,
		std::string_view label,
		char symbol);

	// Removes the entry for `tile`; a tile the palette does not hold leaves it as it was.
	//
	// Example:
	//   palette.remove(cobweb);   // palette.find(cobweb) -> nullopt
	void remove(TileRef tile);

	// The index of the entry for `tile` in entries(), if there is one.
	//
	// Example:
	//   palette.find(cobweb);         // -> 0 in the shipped palette
	//   palette.find(TileRef{});      // -> nullopt
	[[nodiscard]] std::optional<size_t> find(TileRef tile) const;

	// Gives the entry with `symbol` a new tile.
	//
	// Example:
	//   palette.set_symbol_tile('W', unusedTile);   // -> APPLIED
	//   palette.set_symbol_tile('W', barrelTile);   // -> DUPLICATE_TILE, another entry shows that tile
	//   palette.set_symbol_tile('Q', unusedTile);   // -> UNKNOWN_SYMBOL when no entry has 'Q'
	[[nodiscard]] PaletteEdit set_symbol_tile(char symbol, TileRef tile);

	// Gives the entry with `symbol` a new label.
	//
	// Example:
	//   palette.set_symbol_label('W', "Spider web");   // -> APPLIED
	//   palette.set_symbol_label('W', "");             // -> EMPTY_LABEL
	[[nodiscard]] PaletteEdit set_symbol_label(char symbol, std::string_view label);

	// Every entry, in file order.
	//
	// Example:
	//   palette.entries().front().label;   // -> "Cobweb" in the shipped palette
	[[nodiscard]] const std::vector<PaletteEntry>& entries() const noexcept { return paletteEntries; }

	// The symbols a prefab may use: the five structural ones, then each entry that has a
	// symbol, in file order. Built on each call from the entries, so it is never stale.
	//
	// Example:
	//   palette.symbol_rows().size();       // -> 22 in the shipped palette: 5 structural + 17
	//   palette.symbol_rows()[0].label;     // -> "Wall"
	[[nodiscard]] std::vector<SymbolRow> symbol_rows() const;

	// The decoration tile a prefab symbol places, or an invalid TileRef for a structural
	// or unknown symbol, which places nothing.
	//
	// Example:
	//   palette.resolve_decor('W').is_valid();   // -> true
	//   palette.resolve_decor('#').is_valid();   // -> false
	[[nodiscard]] TileRef resolve_decor(char symbol) const;

	// The name a symbol is shown under; an unknown symbol is shown as itself.
	//
	// Example:
	//   palette.symbol_label('#');   // -> "Wall"
	//   palette.symbol_label('Q');   // -> "Q" when no entry has 'Q'
	[[nodiscard]] std::string symbol_label(char symbol) const;
};
