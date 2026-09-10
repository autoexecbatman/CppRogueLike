// Checks that the spawn table can be rebuilt from the item registry, and that rebuilding
// it does not list everything twice.
//
// What it is for. ItemFactory read the registry once, in its constructor, and nothing
// rebuilt it. The item editor writes items.json and reloads ItemCreator while the game is
// running, so an edit could not reach the table until the next launch - in a project whose
// premise is that the editor is the engine, the loop did not close. Map::regenerate now
// rebuilds on the way into a level.
//
// The hazard that comes with it. load_from_registry appends and add_item_type is a bare
// push_back, so a second call without clearing lists every item twice and doubles its
// weight against anything added later. That failure is silent: the game still spawns
// items, just from a table that no longer means what it says. The first test below exists
// for that alone.
//
// The oracle is get_current_distribution, which reports one entry per item with a positive
// weight at the given level. Counting its entries is how a doubled table shows up.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemFactoryReloadTest.*

#include "src/Factories/ItemCreator.h"
#include "src/Factories/ItemFactory.h"
#include <gtest/gtest.h>

#include <string>
#include <vector>

class ItemFactoryReloadTest : public ::testing::Test
{
protected:
	static constexpr int DUNGEON_LEVEL = 1;

	void SetUp() override
	{
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
	}

	void TearDown() override
	{
		// Leave the shared registry as it was found; a weight is changed below.
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
	}
};

// Rebuilding replaces the table rather than appending to it.
TEST_F(ItemFactoryReloadTest, ReloadingDoesNotDuplicateTheTable)
{
	ItemFactory factory;
	const size_t afterConstruction = factory.get_current_distribution(DUNGEON_LEVEL).size();
	ASSERT_GT(afterConstruction, 0u) << "no items in the table; the check would be vacuous";

	factory.reload_from_registry();
	factory.reload_from_registry();

	EXPECT_EQ(factory.get_current_distribution(DUNGEON_LEVEL).size(), afterConstruction)
		<< "the table grew when it was rebuilt, so every item is listed more than once";
}

// A registry change reaches the table, which is the whole reason the reload exists.
TEST_F(ItemFactoryReloadTest, ReloadPicksUpAWeightChange)
{
	ItemFactory factory;

	const std::string key = "health_potion";
	const auto names_in_table = [&]()
	{
		std::vector<std::string> names;
		for (const ItemPercentage& entry : factory.get_current_distribution(DUNGEON_LEVEL))
		{
			names.push_back(entry.name);
		}
		return names;
	};

	const std::vector<std::string> before = names_in_table();
	ASSERT_NE(std::ranges::find(before, ItemCreator::get_params(key).name), before.end())
		<< key << " is not in the table to begin with";

	// Take it out of the spawn table the way an editor would: change the weight.
	ItemParams withoutWeight = ItemCreator::get_params(key);
	withoutWeight.baseWeight = 0;
	ItemCreator::set_params(key, withoutWeight);

	factory.reload_from_registry();

	const std::vector<std::string> after = names_in_table();
	EXPECT_EQ(after.size(), before.size() - 1)
		<< "the weight change never reached the spawn table";
}
