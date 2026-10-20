// file: DecorEditorPaletteTest.cpp
//
// Checks the decoration editor's palette on disk: that it reads every field it needs or
// refuses, and that a save which cannot happen is refused and leaves the file alone. The
// palette lives in tile_config.json beside the tiles, autotile groups and tile types, so a
// writer that met a file it could not parse and wrote a fresh one deleted all of those.
//
// What it does not check: the info bar and browser header that draw a refused save. They
// need a window; issues/run_sheet_editors_0261.txt section L covers them.
//
// Each test works on a copy of the real file in its own temporary file, named after the
// test, so no two tests share one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=DecorEditorPaletteTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/DecorEditor.h"
#include "src/Paths.h"

// Windows marks a file read-only only once every write bit is gone.
constexpr std::filesystem::perms WRITE_PERMISSIONS =
	std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write;

class DecorEditorPaletteTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		scratch = std::filesystem::temp_directory_path() / ("decor_palette_" + testName + ".json");
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

	// What the call threw, or a failure if it threw nothing.
	template <typename EditorCall>
	std::string refusal_of(EditorCall editor_call)
	{
		try
		{
			editor_call();
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << "the call was not refused";
		return {};
	}

	std::filesystem::path scratch{};
	DecorEditor editor{};
};

// The real palette, read and written back over a copy whose palette was emptied, gives the
// real file again: every entry, in order, and every other section untouched.
TEST_F(DecorEditorPaletteTest, TheRealPaletteSurvivesASave)
{
	const nlohmann::json real = read_json(Paths::TILE_CONFIG);
	nlohmann::json emptied = real;
	emptied.at("palette") = nlohmann::json::array();
	write_scratch(emptied.dump(2));

	editor.load_palette(Paths::TILE_CONFIG);
	editor.save_palette(scratch.string());

	EXPECT_EQ(read_json(scratch.string()), real);
}

// Every field of a palette entry, removed from the first one, is named in the refusal.
TEST_F(DecorEditorPaletteTest, AnEntryMissingAFieldIsRefusedByName)
{
	for (const char* field : { "sheet", "col", "row", "label", "symbol" })
	{
		nlohmann::json document = read_json(Paths::TILE_CONFIG);
		document.at("palette").at(0).erase(field);
		write_scratch(document.dump(2));

		const auto load_scratch_palette = [&]()
		{
			editor.load_palette(scratch.string());
		};
		const std::string refusal = refusal_of(load_scratch_palette);

		EXPECT_NE(refusal.find(field), std::string::npos) << "refusal does not name " << field << ": " << refusal;
	}
}

// A palette path that does not open, or a file without its "palette", is refused.
TEST_F(DecorEditorPaletteTest, AMissingFileOrListIsRefused)
{
	const auto load_scratch_palette = [&]()
	{
		editor.load_palette(scratch.string());
	};

	const std::string missingFile = refusal_of(load_scratch_palette);
	EXPECT_NE(missingFile.find(scratch.filename().string()), std::string::npos) << missingFile;

	nlohmann::json document = read_json(Paths::TILE_CONFIG);
	document.erase("palette");
	write_scratch(document.dump(2));
	const std::string missingList = refusal_of(load_scratch_palette);
	EXPECT_NE(missingList.find("palette"), std::string::npos) << missingList;
}

// Saving to a path with no file is refused, and creates nothing: the palette is one
// section of a file it does not own the rest of.
TEST_F(DecorEditorPaletteTest, SavingWhereThereIsNoFileIsRefusedAndCreatesNothing)
{
	editor.load_palette(Paths::TILE_CONFIG);

	const auto save_to_scratch = [&]()
	{
		editor.save_palette(scratch.string());
	};
	const std::string refusal = refusal_of(save_to_scratch);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_NE(refusal.find("cannot open"), std::string::npos) << refusal;
	EXPECT_FALSE(std::filesystem::exists(scratch));
}

// A file that does not parse is refused and left byte for byte, rather than replaced by a
// document holding nothing but the palette.
TEST_F(DecorEditorPaletteTest, SavingOverAFileThatDoesNotParseLeavesIt)
{
	editor.load_palette(Paths::TILE_CONFIG);
	write_scratch("{ \"tiles\": { not json");
	const std::string before = scratch_bytes();

	const auto save_to_scratch = [&]()
	{
		editor.save_palette(scratch.string());
	};
	const std::string refusal = refusal_of(save_to_scratch);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_EQ(scratch_bytes(), before);
}

// A file that cannot be written is refused by name, where the write used to fail silently.
TEST_F(DecorEditorPaletteTest, SavingOverAReadOnlyFileIsRefused)
{
	editor.load_palette(Paths::TILE_CONFIG);
	nlohmann::json emptied = read_json(Paths::TILE_CONFIG);
	emptied.at("palette") = nlohmann::json::array();
	write_scratch(emptied.dump(2));
	const std::string before = scratch_bytes();
	std::filesystem::permissions(scratch, WRITE_PERMISSIONS, std::filesystem::perm_options::remove);

	const auto save_to_scratch = [&]()
	{
		editor.save_palette(scratch.string());
	};
	const std::string refusal = refusal_of(save_to_scratch);

	EXPECT_NE(refusal.find(scratch.filename().string()), std::string::npos) << refusal;
	EXPECT_NE(refusal.find("for writing"), std::string::npos) << refusal;
	EXPECT_EQ(scratch_bytes(), before);
}

// A refused load keeps the palette loaded before it, rather than leaving an editor with
// nothing to place: the real palette is what a later save writes.
TEST_F(DecorEditorPaletteTest, ARefusedLoadKeepsThePaletteBefore)
{
	const nlohmann::json real = read_json(Paths::TILE_CONFIG);
	editor.load_palette(Paths::TILE_CONFIG);
	nlohmann::json broken = real;
	broken.at("palette").at(0).erase("label");
	write_scratch(broken.dump(2));
	const auto load_scratch_palette = [&]()
	{
		editor.load_palette(scratch.string());
	};
	(void)refusal_of(load_scratch_palette);

	nlohmann::json emptied = real;
	emptied.at("palette") = nlohmann::json::array();
	write_scratch(emptied.dump(2));
	editor.save_palette(scratch.string());

	EXPECT_EQ(read_json(scratch.string()).at("palette"), real.at("palette"));
}

// The reporting save keeps a refusal's reason for the editor to draw, and the next save
// that succeeds clears it, so a failure stays on screen exactly as long as it is true.
TEST_F(DecorEditorPaletteTest, AReportedRefusalStaysUntilASaveSucceeds)
{
	const std::string realText = read_json(Paths::TILE_CONFIG).dump(2);
	write_scratch(realText);
	editor.load_palette(scratch.string());
	ASSERT_FALSE(editor.save_error().has_value());

	write_scratch("{ not json");
	editor.save_palette_reporting();
	ASSERT_TRUE(editor.save_error().has_value());
	EXPECT_NE(editor.save_error()->find(scratch.filename().string()), std::string::npos) << *editor.save_error();

	write_scratch(realText);
	editor.save_palette_reporting();
	EXPECT_FALSE(editor.save_error().has_value()) << *editor.save_error();
}
