// file: ItemSpawnTableTest.cpp
// The item spawn table is the registry's, derived when an item is drawn. Nothing keeps a
// copy of it, so an edit made in the item editor reaches the next spawn, and a game
// loaded from a save spawns from the registry main loaded - neither waits for a new
// level to rebuild anything. The factory used to build its table once, in its
// constructor, and a loaded game never passed through the level change that rebuilt it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemSpawnTableTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "src/Item.h"
#include "src/ItemFactory.h"
#include "src/ItemRegistry.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int DUNGEON_LEVEL = 1;

// Whether a distribution lists an entry by name.
bool lists(const std::vector<ItemPercentage>& distribution, std::string_view name)
{
	auto is_named = [name](const ItemPercentage& entry)
	{
		return entry.name == name;
	};
	return std::ranges::any_of(distribution, is_named);
}

// The first item the registry draws at a level: positive weight, and a level range that
// admits it, where a levelMax of 0 means no upper bound. Chosen from the data, since an
// item named by hand may not be drawn at that level at all.
std::string first_drawable(const ItemRegistry& items, int dungeonLevel)
{
	for (const std::string& key : items.get_all_keys())
	{
		const ItemParams& params = items.get_params(key);
		const bool admitsLevel = params.levelMin <= dungeonLevel && (params.levelMax == 0 || dungeonLevel <= params.levelMax);
		if (params.baseWeight > 0 && admitsLevel)
		{
			return key;
		}
	}
	return {};
}
} // namespace

// Silencing an item in the registry takes it out of the very next distribution, with
// nothing rebuilt in between.
TEST(ItemSpawnTableTest, TheDistributionIsTheRegistrysAsItStandsNow)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);
	items.load_enhanced_rules(Paths::ENHANCED_RULES);
	const std::string target = first_drawable(items, DUNGEON_LEVEL);
	ASSERT_FALSE(target.empty()) << "no item is drawable at this level to silence";
	const std::string name{ items.get_params(target).name };
	ASSERT_TRUE(lists(ItemFactory::get_current_distribution(DUNGEON_LEVEL, items), name));

	ItemParams silenced = items.get_params(target);
	silenced.baseWeight = 0;
	items.set_params(target, silenced);

	EXPECT_FALSE(lists(ItemFactory::get_current_distribution(DUNGEON_LEVEL, items), name))
		<< "silencing " << target << " never reached the distribution";
}

// An item the editor adds joins the table at once.
TEST(ItemSpawnTableTest, ACustomItemJoinsTheTable)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);

	[[maybe_unused]] const std::string key = items.add_custom("Registry Trinket", "potion", ItemParams{ .baseWeight = 10, .levelMin = 1 });

	EXPECT_TRUE(lists(ItemFactory::get_current_distribution(DUNGEON_LEVEL, items), "Registry Trinket"));
}

// A spawn draws from the registry in the ctx it is given. Two contexts, each with every
// potion but one silenced, and a roll of 1 in each: the first places its health potion
// and the second its invisibility potion.
TEST(ItemSpawnTableTest, ASpawnDrawsFromTheRegistryInCtx)
{
	auto spawn_with_only = [](std::string_view survivor)
	{
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		for (const std::string& key : mock.itemRegistry.get_all_keys())
		{
			ItemParams params = mock.itemRegistry.get_params(key);
			if (params.category == "potion" && key != survivor)
			{
				params.baseWeight = 0;
				mock.itemRegistry.set_params(key, params);
			}
		}
		mock.dice.set_next_roll(1);

		ItemFactory::spawn_item_of_category(Vector2D{ 1, 1 }, DUNGEON_LEVEL, "potion", ctx);

		return mock.inventory.items.size() == 1 ? mock.inventory.items.front()->itemKey : std::string{ "placed nothing, or more than one" };
	};

	EXPECT_EQ(spawn_with_only("health_potion"), "health_potion");
	EXPECT_EQ(spawn_with_only("invisibility_potion"), "invisibility_potion");
}
