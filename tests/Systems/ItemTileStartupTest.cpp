// file: ItemTileStartupTest.cpp
// The item tiles startup loads, and that saving them keeps them all. The
// editors open from the main menu before any run starts, so the tiles have to
// be loaded at startup: with an empty registry the item editor's list draws no
// tiles, and its Ctrl+S writes the empty registry over tiles.json.
//
// Every expected tile is read from the shipped data/content/tiles.json, so the
// claim is "all of the file", not a count written here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemTileStartupTest.*

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "src/ContentRegistryIO.h"
#include "src/Game.h"
#include "src/Paths.h"
#include "src/TileRef.h"
#include "src/TileRefJson.h"
#include "tests/mocks/TestDice.h"

namespace
{
	// The item tiles the game ships, by key, as tiles.json holds them.
	nlohmann::json shipped_item_tiles()
	{
		std::ifstream in(Paths::resolve(Paths::CONTENT_TILES));
		return nlohmann::json::parse(in).at("items");
	}
} // namespace

TEST(ItemTileStartupTest, StartupLoadsEveryShippedItemTile)
{
	const nlohmann::json shipped = shipped_item_tiles();
	ASSERT_FALSE(shipped.empty()) << "tiles.json ships no item tiles";
	Game game{ TEST_DICE_SEED };

	game.load_item_tiles();

	EXPECT_EQ(game.contentRegistry.all_tiles().size(), shipped.size());
	for (const auto& [key, record] : shipped.items())
	{
		EXPECT_EQ(
			game.contentRegistry.get_tile(key),
			TileRefJson::parse_tile(record))
			<< key;
	}
}

TEST(ItemTileStartupTest, SavingTheLoadedTilesKeepsThemAll)
{
	const nlohmann::json shipped = shipped_item_tiles();
	Game game{ TEST_DICE_SEED };
	game.load_item_tiles();
	const std::filesystem::path saved =
		std::filesystem::temp_directory_path() / "rogue_item_tiles_test.json";

	ContentRegistryIO::save(game.contentRegistry, saved.string());
	std::ifstream in(saved);
	const nlohmann::json written = nlohmann::json::parse(in).at("items");
	in.close();
	std::error_code ignored;
	std::filesystem::remove(saved, ignored);

	EXPECT_EQ(written, shipped);
}
