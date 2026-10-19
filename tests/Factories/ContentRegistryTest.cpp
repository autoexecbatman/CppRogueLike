// file: ContentRegistryTest.cpp
// The registry of item sprites, keyed by item key. A sprite is removed with the item it
// belongs to, and removing one the registry never held is the caller's mistake.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ContentRegistryTest.*

#include <gtest/gtest.h>

#include <stdexcept>

#include "src/ContentRegistry.h"
#include "src/Renderer.h"

// A removed sprite is no longer listed, and the key reads the null sentinel again.
TEST(ContentRegistryTest, ARemovedTileIsGone)
{
	ContentRegistry tiles{};
	tiles.set_tile("registry_trinket", TileRef{ TileSheet::SHEET_POTION, 1, 2 });
	tiles.set_tile("health_potion", TileRef{ TileSheet::SHEET_POTION, 0, 0 });

	tiles.remove_tile("registry_trinket");

	EXPECT_FALSE(tiles.all_tiles().contains("registry_trinket"));
	EXPECT_EQ(tiles.get_tile("registry_trinket"), TileRef{});
	EXPECT_TRUE(tiles.all_tiles().contains("health_potion")) << "removing one sprite removed another";
}

// Removing a sprite nobody assigned throws rather than passing silently.
TEST(ContentRegistryTest, RemovingAnUnknownTileThrows)
{
	ContentRegistry tiles{};

	EXPECT_THROW(tiles.remove_tile("no_such_item"), std::out_of_range);
}
