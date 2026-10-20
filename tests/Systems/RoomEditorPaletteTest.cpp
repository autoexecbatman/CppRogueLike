// file: RoomEditorPaletteTest.cpp
//
// Checks that the room editor's two palette edits - a symbol's label (F3) and a symbol's tile
// (the F2 picker) - are saved to the palette's file, and that the status line names an edit
// only after its save happened. Both edits used to report success after writing prefabs.json,
// which does not hold the palette, so they were gone on the next start (issues/0290).
//
// What it does not check: the keys that lead to these calls and the drawn status line. They
// need a window; the calls under test are the ones those keys make.
//
// Each test edits a copy of the real tile_config.json in its own temporary file, named after
// the test, so no test writes data/.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=RoomEditorPaletteTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/Palette.h"
#include "src/Paths.h"
#include "src/PrefabLibrary.h"
#include "src/RoomEditor.h"
#include "src/TileRefJson.h"

namespace
{
// Windows marks a file read-only only once every write bit is gone.
constexpr std::filesystem::perms ROOM_EDITOR_WRITE_PERMISSIONS =
	std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write;
} // namespace

class RoomEditorPaletteTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		scratch = std::filesystem::temp_directory_path() / ("room_editor_palette_" + testName + ".json");

		std::ifstream source(Paths::resolve(Paths::TILE_CONFIG));
		ASSERT_TRUE(source.is_open());
		real = nlohmann::json::parse(source);
		std::ofstream out(scratch, std::ios::binary);
		out << real.dump(2);
		out.close();

		palette.load(scratch.string());
		editor.enter(library, palette);
	}

	void TearDown() override
	{
		// A read-only file cannot be removed on Windows, so its write permission comes back first.
		if (std::filesystem::exists(scratch))
		{
			std::filesystem::permissions(scratch, ROOM_EDITOR_WRITE_PERMISSIONS, std::filesystem::perm_options::add);
			std::filesystem::remove(scratch);
		}
	}

	[[nodiscard]] std::string scratch_bytes() const
	{
		std::ifstream source(scratch, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
	}

	// The first real entry with a symbol, which both edits address.
	[[nodiscard]] const nlohmann::json& symbolled_entry() const
	{
		for (const nlohmann::json& entry : real.at("palette"))
		{
			if (!entry.at("symbol").get<std::string>().empty())
			{
				return entry;
			}
		}
		ADD_FAILURE() << "no palette entry has a symbol";
		return real.at("palette").at(0);
	}

	[[nodiscard]] char symbol() const
	{
		return symbolled_entry().at("symbol").get<std::string>().at(0);
	}

	// A tile on the symbol's own sheet that no real entry shows.
	[[nodiscard]] TileRef unused_tile() const
	{
		TileRef candidate = TileRefJson::parse_tile(symbolled_entry());
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
		ADD_FAILURE() << "no unused tile on the symbol's sheet";
		return TileRef{};
	}

	// What a fresh palette reads back from the scratch file.
	[[nodiscard]] Palette reloaded() const
	{
		Palette fresh;
		fresh.load(scratch.string());
		return fresh;
	}

	void make_scratch_read_only() const
	{
		std::filesystem::permissions(scratch, ROOM_EDITOR_WRITE_PERMISSIONS, std::filesystem::perm_options::remove);
	}

	std::filesystem::path scratch{};
	nlohmann::json real{};
	Palette palette{};
	PrefabLibrary library{};
	RoomEditor editor{};
};

// The issue's check: a label typed in the room editor is in the file, and the status says so.
TEST_F(RoomEditorPaletteTest, ALabelEditIsSavedToThePaletteFile)
{
	editor.commit_symbol_label(symbol(), "Renamed in the room editor");

	EXPECT_EQ(reloaded().symbol_label(symbol()), "Renamed in the room editor");
	EXPECT_EQ(editor.status(), std::format("Label: '{}'=Renamed in the room editor", symbol()));
}

// The same for a tile picked for a symbol: the symbol places the new tile after a reload.
TEST_F(RoomEditorPaletteTest, ATilePickIsSavedToThePaletteFile)
{
	const TileRef picked = unused_tile();

	editor.commit_symbol_tile(symbol(), picked);

	EXPECT_EQ(reloaded().resolve_decor(symbol()), picked);
	EXPECT_NE(editor.status().find(std::format("'{}' -> tile ({},{})", symbol(), picked.col, picked.row)), std::string::npos)
		<< editor.status();
}

// A save that cannot happen is named in the status, and the edit's success is not.
TEST_F(RoomEditorPaletteTest, AnEditWhoseSaveIsRefusedSaysSoAndNotThatItSucceeded)
{
	make_scratch_read_only();
	// The fixture's premise, shown rather than assumed: this file refuses a write.
	EXPECT_THROW(palette.save(), std::runtime_error);

	editor.commit_symbol_label(symbol(), "Never saved");
	EXPECT_EQ(editor.status().rfind("SAVE FAILED: ", 0), 0u) << editor.status();
	EXPECT_EQ(editor.status().find("Label:"), std::string::npos) << editor.status();

	editor.commit_symbol_tile(symbol(), unused_tile());
	EXPECT_EQ(editor.status().rfind("SAVE FAILED: ", 0), 0u) << editor.status();
	EXPECT_EQ(editor.status().find("-> tile"), std::string::npos) << editor.status();
}

// An edit the palette refuses writes nothing and says why.
TEST_F(RoomEditorPaletteTest, ARefusedEditSavesNothingAndSaysWhy)
{
	const std::string before = scratch_bytes();

	editor.commit_symbol_label('#', "Stone");
	EXPECT_EQ(editor.status(), std::format("NOT SAVED: {}", palette_edit_refusal(PaletteEdit::STRUCTURAL_SYMBOL)));

	editor.commit_symbol_label(symbol(), "");
	EXPECT_EQ(editor.status(), std::format("NOT SAVED: {}", palette_edit_refusal(PaletteEdit::EMPTY_LABEL)));

	editor.commit_symbol_tile('#', unused_tile());
	EXPECT_EQ(editor.status(), std::format("NOT SAVED: {}", palette_edit_refusal(PaletteEdit::STRUCTURAL_SYMBOL)));

	EXPECT_EQ(scratch_bytes(), before);
}
