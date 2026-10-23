// file: ItemTileAgreementTest.cpp
// items.json and the "items" section of tiles.json each state which items exist. A sprite
// with no item, or an item with no sprite, is a hand edit or a stale editor save, and this
// names the key either way.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemTileAgreementTest.*

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "src/ContentRegistry.h"
#include "src/ContentRegistryIO.h"
#include "src/ItemRegistry.h"
#include "src/Paths.h"

// Every shipped item has a sprite, and every sprite belongs to a shipped item.
TEST(ItemTileAgreementTest, EveryShippedItemHasATileAndEveryTileAnItem)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);
	ContentRegistry tiles{};
	ContentRegistryIO::load(tiles, Paths::CONTENT_TILES);

	const std::vector<std::string> keys = items.get_all_keys();
	const std::set<std::string> itemKeys(keys.begin(), keys.end());
	ASSERT_FALSE(itemKeys.empty())
		<< "no items loaded; the comparison would pass vacuously";

	for (const std::string& key : itemKeys)
	{
		EXPECT_TRUE(tiles.all_tiles().contains(key))
			<< "item '" << key << "' has no tile";
	}
	for (const auto& [key, tile] : tiles.all_tiles())
	{
		EXPECT_TRUE(itemKeys.contains(key))
			<< "tile '" << key << "' belongs to no item";
	}
}
