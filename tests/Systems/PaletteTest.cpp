// file: PaletteTest.cpp
//
// Checks the one decoration palette: that it reads every field it needs or refuses, that a
// save which cannot happen is refused and leaves the file alone, that an edit breaking one of
// its four invariants is refused and changes nothing, and that an edit survives a save and a
// fresh load. The palette lives in tile_config.json beside the tiles, autotile groups and tile
// types, so a writer that met a file it could not parse and wrote a fresh one deleted all of
// those.
//
// What it does not check: the info bar and browser header that draw a refused save or edit.
// They need a window; issues/run_sheet_editors_0261.txt section L covers them.
//
// Each test works on a copy of the real file in its own temporary file, named after the
// test, so no two tests share one and no test writes data/.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=PaletteTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "src/DecorEditor.h"
#include "src/Palette.h"
#include "src/Paths.h"
#include "src/TileRefJson.h"

// Windows marks a file read-only only once every write bit is gone.
constexpr std::filesystem::perms WRITE_PERMISSIONS =
	std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write;

class PaletteTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		scratch = std::filesystem::temp_directory_path() / ("palette_" + testName + ".json");
		real = read_json(Paths::TILE_CONFIG);
	}

	void TearDown() override
	{
		// A read-only file cannot be removed on Windows, so its write permission comes back first.
		if (std::filesystem::exists(scratch))
		{
			std::filesystem::permissions(scratch, WRITE_PERMISSIONS, std::filesystem::perm_options::add);
			std::filesystem::remove(scratch);
		}
	}

	static nlohmann::json read_json(std::string_view path)
	{
		std::ifstream source(Paths::resolve(path));
		EXPECT_TRUE(source.is_open()) << "cannot read " << path;
		return nlohmann::json::parse(source);
	}

	void write_scratch(const std::string& text) const
	{
		std::ofstream out(scratch, std::ios::binary);
		out << text;
	}

	[[nodiscard]] std::string scratch_bytes() const
	{
		std::ifstream source(scratch, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
	}

	// Loads a copy of the real file, so save() writes the scratch file and never data/.
	void load_real_copy()
	{
		write_scratch(real.dump(2));
		palette.load(scratch.string());
	}

	// A copy of the real file whose palette is `entries`.
	void write_palette(const nlohmann::json& entries) const
	{
		nlohmann::json document = real;
		document.at("palette") = entries;
		write_scratch(document.dump(2));
	}

	// The real entries that carry a symbol, in file order.
	[[nodiscard]] std::vector<nlohmann::json> symbolled_entries() const
	{
		std::vector<nlohmann::json> symbolled;
		for (const nlohmann::json& entry : real.at("palette"))
		{
			if (!entry.at("symbol").get<std::string>().empty())
			{
				symbolled.push_back(entry);
			}
		}
		return symbolled;
	}

	[[nodiscard]] static char symbol_of(const nlohmann::json& entry)
	{
		return entry.at("symbol").get<std::string>().at(0);
	}

	// A printable symbol no real entry and no structural symbol uses.
	[[nodiscard]] char unused_symbol() const
	{
		for (char candidate = 'A'; candidate <= 'Z'; ++candidate)
		{
			bool isTaken = false;
			for (const nlohmann::json& entry : symbolled_entries())
			{
				isTaken = isTaken || symbol_of(entry) == candidate;
			}
			if (!isTaken)
			{
				return candidate;
			}
		}
		ADD_FAILURE() << "every capital letter is a symbol";
		return 0;
	}

	// A tile on the first entry's sheet that no real entry shows.
	[[nodiscard]] TileRef unused_tile() const
	{
		TileRef candidate = TileRefJson::parse_tile(real.at("palette").at(0));
		for (int col = 0; col < 64; ++col)
		{
			candidate.col = col;
			bool isTaken = false;
			for (const nlohmann::json& entry : real.at("palette"))
			{
				isTaken = isTaken || TileRefJson::parse_tile(entry) == candidate;
			}
			if (!isTaken)
			{
				return candidate;
			}
		}
		ADD_FAILURE() << "no unused tile on the first entry's sheet";
		return TileRef{};
	}

	// What the call threw, or a failure if it threw nothing.
	template <typename PaletteCall>
	std::string refusal_of(PaletteCall palette_call)
	{
		try
		{
			palette_call();
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << "the call was not refused";
		return {};
	}

	std::filesystem::path scratch{};
	nlohmann::json real{};
	Palette palette{};
};

// ---------------------------------------------------------------------------
// Loading and saving
// ---------------------------------------------------------------------------

// The real palette, read and written back over a copy whose palette was emptied, gives the
// real file again: every entry, in order, and every other section untouched.
TEST_F(PaletteTest, TheRealPaletteSurvivesASave)
{
	load_real_copy();
	write_palette(nlohmann::json::array());

	palette.save();

	EXPECT_EQ(read_json(scratch.string()), real);
}

// Every real entry is loaded, unlabelled or without a symbol alike, in file order.
TEST_F(PaletteTest, EveryRealEntryLoadsInOrder)
{
	load_real_copy();

	ASSERT_EQ(palette.entries().size(), real.at("palette").size());
	for (size_t entryIndex = 0; entryIndex < palette.entries().size(); ++entryIndex)
	{
		const nlohmann::json& entry = real.at("palette").at(entryIndex);
		EXPECT_EQ(palette.entries()[entryIndex].tile, TileRefJson::parse_tile(entry)) << entryIndex;
		EXPECT_EQ(palette.entries()[entryIndex].label, entry.at("label").get<std::string>()) << entryIndex;
	}
}

// Every field of a palette entry, removed from the first one, is named in the refusal.
TEST_F(PaletteTest, AnEntryMissingAFieldIsRefusedByName)
{
	for (const char* field : { "sheet", "col", "row", "label", "symbol" })
	{
		nlohmann::json entries = real.at("palette");
		entries.at(0).erase(field);
		write_palette(entries);

		const auto load_scratch_palette = [&]()
		{
			palette.load(scratch.string());
		};
		const std::string refusal = refusal_of(load_scratch_palette);

		EXPECT_NE(refusal.find(field), std::string::npos) << "refusal does not name " << field << ": " << refusal;
	}
}

// A palette path that does not open, or a file without its "palette", is refused.
TEST_F(PaletteTest, AMissingFileOrListIsRefused)
{
	const auto load_scratch_palette = [&]()
	{
		palette.load(scratch.string());
	};

	const std::string missingFile = refusal_of(load_scratch_palette);
	EXPECT_NE(missingFile.find(scratch.filename().string()), std::string::npos) << missingFile;

	nlohmann::json document = real;
	document.erase("palette");
	write_scratch(document.dump(2));
	const std::string missingList = refusal_of(load_scratch_palette);
	EXPECT_NE(missingList.find("palette"), std::string::npos) << missingList;
}

// A file whose entries break an invariant is refused naming the entry's label, and the
// palette loaded before is kept.
TEST_F(PaletteTest, AFileBreakingAnInvariantIsRefusedByEntry)
{
	const std::vector<nlohmann::json> symbolled = symbolled_entries();
	ASSERT_GE(symbolled.size(), 2u);

	nlohmann::json duplicateSymbol = real.at("palette");
	duplicateSymbol.push_back(symbolled[1]);
	duplicateSymbol.back()["col"] = unused_tile().col;
	duplicateSymbol.back()["sheet"] = static_cast<int>(unused_tile().sheet);
	duplicateSymbol.back()["row"] = unused_tile().row;
	duplicateSymbol.back()["label"] = "Second copy";
	duplicateSymbol.back()["symbol"] = std::string(1, symbol_of(symbolled[0]));

	nlohmann::json duplicateTile = real.at("palette");
	duplicateTile.push_back(real.at("palette").at(0));
	duplicateTile.back()["label"] = "Second copy";
	duplicateTile.back()["symbol"] = "";

	nlohmann::json structural = real.at("palette");
	structural.at(0)["symbol"] = "#";
	structural.at(0)["label"] = "Second copy";

	nlohmann::json emptyLabel = real.at("palette");
	emptyLabel.at(0)["label"] = "";

	load_real_copy();
	const std::vector<PaletteEntry> before = palette.entries();
	const auto load_scratch_palette = [&]()
	{
		palette.load(scratch.string());
	};

	for (const nlohmann::json& broken : { duplicateSymbol, duplicateTile, structural })
	{
		write_palette(broken);
		const std::string refusal = refusal_of(load_scratch_palette);
		EXPECT_NE(refusal.find("Second copy"), std::string::npos) << refusal;
		EXPECT_EQ(palette.entries(), before);
	}

	write_palette(emptyLabel);
	const std::string refusal = refusal_of(load_scratch_palette);
	EXPECT_NE(refusal.find("empty label"), std::string::npos) << refusal;
	EXPECT_EQ(palette.entries(), before);
}

// Saving where there is no file is refused, and creates nothing: the palette is one
// section of a file it does not own the rest of.
TEST_F(PaletteTest, SavingWhereThereIsNoFileIsRefusedAndCreatesNothing)
{
	load_real_copy();
	std::filesystem::remove(scratch);

	const auto save_palette = [&]()
	{
		palette.save();
	};
	const std::string refusal = refusal_of(save_palette);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_NE(refusal.find("cannot open"), std::string::npos) << refusal;
	EXPECT_FALSE(std::filesystem::exists(scratch));
}

// A file that does not parse is refused and left byte for byte, rather than replaced by a
// document holding nothing but the palette.
TEST_F(PaletteTest, SavingOverAFileThatDoesNotParseLeavesIt)
{
	load_real_copy();
	write_scratch("{ \"tiles\": { not json");
	const std::string before = scratch_bytes();

	const auto save_palette = [&]()
	{
		palette.save();
	};
	const std::string refusal = refusal_of(save_palette);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_EQ(scratch_bytes(), before);
}

// A file that cannot be written is refused by name, where the write used to fail silently.
TEST_F(PaletteTest, SavingOverAReadOnlyFileIsRefused)
{
	load_real_copy();
	write_palette(nlohmann::json::array());
	const std::string before = scratch_bytes();
	std::filesystem::permissions(scratch, WRITE_PERMISSIONS, std::filesystem::perm_options::remove);

	const auto save_palette = [&]()
	{
		palette.save();
	};
	const std::string refusal = refusal_of(save_palette);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_NE(refusal.find("for writing"), std::string::npos) << refusal;
	EXPECT_EQ(scratch_bytes(), before);
}

// A refused load keeps the palette loaded before it, rather than leaving an editor with
// nothing to place: the real palette is what a later save writes.
TEST_F(PaletteTest, ARefusedLoadKeepsThePaletteBefore)
{
	load_real_copy();
	nlohmann::json broken = real.at("palette");
	broken.at(0).erase("label");
	write_palette(broken);
	const auto load_scratch_palette = [&]()
	{
		palette.load(scratch.string());
	};
	(void)refusal_of(load_scratch_palette);

	write_palette(nlohmann::json::array());
	palette.save();

	EXPECT_EQ(read_json(scratch.string()).at("palette"), real.at("palette"));
}

// ---------------------------------------------------------------------------
// Edits survive a save
// ---------------------------------------------------------------------------

// The issue's own check: a label edited through the palette, saved and read by a fresh
// palette from the same file, comes back as edited.
TEST_F(PaletteTest, ALabelEditedBySymbolSurvivesASaveAndReload)
{
	const char symbol = symbol_of(symbolled_entries().at(0));
	load_real_copy();

	ASSERT_EQ(palette.set_symbol_label(symbol, "Renamed in a test"), PaletteEdit::APPLIED);
	palette.save();

	Palette reloaded;
	reloaded.load(scratch.string());
	EXPECT_EQ(reloaded.symbol_label(symbol), "Renamed in a test");
}

// The same for a tile picked for a symbol: the symbol places the new tile after a reload.
TEST_F(PaletteTest, ATilePickedForASymbolSurvivesASaveAndReload)
{
	const char symbol = symbol_of(symbolled_entries().at(0));
	const TileRef picked = unused_tile();
	load_real_copy();

	ASSERT_EQ(palette.set_symbol_tile(symbol, picked), PaletteEdit::APPLIED);
	palette.save();

	Palette reloaded;
	reloaded.load(scratch.string());
	EXPECT_EQ(reloaded.resolve_decor(symbol), picked);
}

// An entry added by tile is kept at the end, and one updated by tile changes in place.
TEST_F(PaletteTest, AddingByTileAppendsAndUpdatingChangesInPlace)
{
	load_real_copy();
	const size_t count = real.at("palette").size();
	const TileRef first = TileRefJson::parse_tile(real.at("palette").at(0));
	const TileRef added = unused_tile();
	const char newSymbol = unused_symbol();

	ASSERT_EQ(palette.add_or_update(added, "Added", newSymbol), PaletteEdit::APPLIED);
	ASSERT_EQ(palette.add_or_update(first, "Updated", 0), PaletteEdit::APPLIED);

	ASSERT_EQ(palette.entries().size(), count + 1);
	EXPECT_EQ(palette.find(added), count);
	EXPECT_EQ(palette.entries().back().label, "Added");
	EXPECT_EQ(palette.entries().back().symbol, newSymbol);
	EXPECT_EQ(palette.find(first), 0u);
	EXPECT_EQ(palette.entries().front().label, "Updated");
	EXPECT_EQ(palette.entries().front().symbol, 0);
}

// Keeping an entry's own symbol while renaming it is not a duplicate.
TEST_F(PaletteTest, AnEntryMayKeepItsOwnSymbol)
{
	const nlohmann::json entry = symbolled_entries().at(0);
	load_real_copy();

	EXPECT_EQ(palette.add_or_update(TileRefJson::parse_tile(entry), "Same symbol", symbol_of(entry)), PaletteEdit::APPLIED);
	EXPECT_EQ(palette.set_symbol_tile(symbol_of(entry), TileRefJson::parse_tile(entry)), PaletteEdit::APPLIED);
}

// Removing a tile takes its entry out; a tile the palette does not hold changes nothing.
TEST_F(PaletteTest, RemovingATileTakesOutItsEntryOnly)
{
	load_real_copy();
	const TileRef first = TileRefJson::parse_tile(real.at("palette").at(0));
	const size_t count = real.at("palette").size();

	palette.remove(unused_tile());
	EXPECT_EQ(palette.entries().size(), count);

	palette.remove(first);
	EXPECT_EQ(palette.entries().size(), count - 1);
	EXPECT_FALSE(palette.find(first).has_value());
}

// ---------------------------------------------------------------------------
// Refused edits change nothing
// ---------------------------------------------------------------------------

// Each edit that would break an invariant names the invariant and leaves every entry as
// it was.
TEST_F(PaletteTest, AnEditBreakingAnInvariantIsRefusedAndChangesNothing)
{
	const std::vector<nlohmann::json> symbolled = symbolled_entries();
	ASSERT_GE(symbolled.size(), 2u);
	const char firstSymbol = symbol_of(symbolled[0]);
	const TileRef secondTile = TileRefJson::parse_tile(symbolled[1]);
	load_real_copy();
	const std::vector<PaletteEntry> before = palette.entries();

	EXPECT_EQ(palette.add_or_update(secondTile, "Clash", firstSymbol), PaletteEdit::DUPLICATE_SYMBOL);
	EXPECT_EQ(palette.add_or_update(unused_tile(), "Clash", firstSymbol), PaletteEdit::DUPLICATE_SYMBOL);
	EXPECT_EQ(palette.set_symbol_tile(firstSymbol, secondTile), PaletteEdit::DUPLICATE_TILE);
	EXPECT_EQ(palette.add_or_update(unused_tile(), "Wall again", '#'), PaletteEdit::STRUCTURAL_SYMBOL);
	EXPECT_EQ(palette.set_symbol_label('~', "Lake"), PaletteEdit::STRUCTURAL_SYMBOL);
	EXPECT_EQ(palette.set_symbol_tile('+', unused_tile()), PaletteEdit::STRUCTURAL_SYMBOL);
	EXPECT_EQ(palette.set_symbol_label(unused_symbol(), "Nobody"), PaletteEdit::UNKNOWN_SYMBOL);
	EXPECT_EQ(palette.set_symbol_tile(unused_symbol(), unused_tile()), PaletteEdit::UNKNOWN_SYMBOL);
	EXPECT_EQ(palette.add_or_update(unused_tile(), "", 0), PaletteEdit::EMPTY_LABEL);
	EXPECT_EQ(palette.set_symbol_label(firstSymbol, ""), PaletteEdit::EMPTY_LABEL);

	EXPECT_EQ(palette.entries(), before);
}

// Every refusal has a sentence for the editor to show, and no two share one.
TEST_F(PaletteTest, EveryRefusalHasItsOwnSentence)
{
	const std::vector<PaletteEdit> refusals{
		PaletteEdit::DUPLICATE_SYMBOL,
		PaletteEdit::DUPLICATE_TILE,
		PaletteEdit::STRUCTURAL_SYMBOL,
		PaletteEdit::UNKNOWN_SYMBOL,
		PaletteEdit::EMPTY_LABEL,
	};
	for (size_t outer = 0; outer < refusals.size(); ++outer)
	{
		EXPECT_FALSE(palette_edit_refusal(refusals[outer]).empty()) << outer;
		for (size_t inner = outer + 1; inner < refusals.size(); ++inner)
		{
			EXPECT_NE(palette_edit_refusal(refusals[outer]), palette_edit_refusal(refusals[inner])) << outer << " " << inner;
		}
	}
}

// ---------------------------------------------------------------------------
// Symbol lookups
// ---------------------------------------------------------------------------

// The prefab symbol list is the five structural symbols, then every entry with a symbol,
// counted from the file.
TEST_F(PaletteTest, SymbolRowsAreTheStructuralFiveThenEachSymbolledEntry)
{
	const std::vector<nlohmann::json> symbolled = symbolled_entries();
	load_real_copy();

	const std::vector<SymbolRow> rows = palette.symbol_rows();

	ASSERT_EQ(rows.size(), STRUCTURAL_SYMBOLS.size() + symbolled.size());
	for (size_t rowIndex = 0; rowIndex < STRUCTURAL_SYMBOLS.size(); ++rowIndex)
	{
		EXPECT_EQ(rows[rowIndex].symbol, STRUCTURAL_SYMBOLS[rowIndex].symbol);
		EXPECT_EQ(rows[rowIndex].label, STRUCTURAL_SYMBOLS[rowIndex].label);
	}
	for (size_t entryIndex = 0; entryIndex < symbolled.size(); ++entryIndex)
	{
		const SymbolRow& row = rows[STRUCTURAL_SYMBOLS.size() + entryIndex];
		EXPECT_EQ(row.symbol, symbol_of(symbolled[entryIndex]));
		EXPECT_EQ(row.label, symbolled[entryIndex].at("label").get<std::string>());
	}
}

// A symbol resolves to its entry's tile and name; a structural symbol to no tile and its
// fixed name; an unknown one to no tile and itself.
TEST_F(PaletteTest, ASymbolResolvesToItsEntryAndOthersToNoTile)
{
	const nlohmann::json entry = symbolled_entries().at(0);
	const char unknown = unused_symbol();
	load_real_copy();

	EXPECT_EQ(palette.resolve_decor(symbol_of(entry)), TileRefJson::parse_tile(entry));
	EXPECT_EQ(palette.symbol_label(symbol_of(entry)), entry.at("label").get<std::string>());

	EXPECT_FALSE(palette.resolve_decor('#').is_valid());
	EXPECT_EQ(palette.symbol_label('#'), "Wall");

	EXPECT_FALSE(palette.resolve_decor(unknown).is_valid());
	EXPECT_EQ(palette.symbol_label(unknown), std::string(1, unknown));
}

// ---------------------------------------------------------------------------
// The decoration editor's reporting save
// ---------------------------------------------------------------------------

// The reporting save keeps a refusal's reason for the editor to draw, and the next save
// that succeeds clears it, so a failure stays on screen exactly as long as it is true.
TEST_F(PaletteTest, AReportedRefusalStaysUntilASaveSucceeds)
{
	DecorEditor editor{ palette };
	load_real_copy();
	ASSERT_FALSE(editor.save_error().has_value());

	write_scratch("{ not json");
	editor.save_palette_reporting();
	ASSERT_TRUE(editor.save_error().has_value());
	EXPECT_NE(editor.save_error()->find(scratch.filename().string()), std::string::npos) << *editor.save_error();

	write_scratch(real.dump(2));
	editor.save_palette_reporting();
	EXPECT_FALSE(editor.save_error().has_value()) << *editor.save_error();
}
