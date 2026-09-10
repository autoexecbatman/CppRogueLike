// Checks that saving items.json and reading it back preserves what was authored.
//
// What it is for. items.json is the file this project has already lost once: the data
// used snake_case keys while the parser read camelCase, every field silently took its
// default, and create_random_of_category returned nullptr forever because baseWeight came
// back 0. Nothing threw and nothing logged. parse_item_entry still reads all thirty-four
// of its keys with j.value(), so that failure is still available - every field here can
// vanish without a sound, which makes this the file where a round-trip earns the most.
//
// What it deliberately does not check. Not that values are correct, only that they
// survive. Not the enhanced spawn rules, which load from a separate file through
// load_enhanced_rules and have their own schema.
//
// A trap this file had to work around, worth knowing before editing it. ItemParams holds
// name and category as std::string_view pointing into the ItemEntry that owns them, and
// load() clears the registry. A params copied by value before a reload therefore has two
// dangling views afterwards, and comparing them reads freed memory. The owned strings are
// copied out separately below.
//
// The oracle is parse_item_entry, the only schema this data has: every key the parser
// reads is a key the encoder must write. Expected values come from the entry loaded
// before the save, never from what the encoder produces.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemRoundTripTest.*

#include "src/Factories/ItemCreator.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace
{

// An item as it stood before the save, with the two viewed strings owned so they survive
// the registry being cleared.
struct ItemSnapshot
{
	std::string name{};
	std::string category{};
	ItemParams params{};
};

}

class ItemRoundTripTest : public ::testing::Test
{
protected:
	std::filesystem::path roundTrip;

	void SetUp() override
	{
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
		roundTrip = std::filesystem::temp_directory_path() / "items_roundtrip.json";
	}

	void TearDown() override
	{
		// Leave the shared registry holding the real file, not the temporary one.
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
		std::filesystem::remove(roundTrip);
	}
};

// Every field ItemParams declares, named one at a time, so a lost key fails pointing at
// the field rather than at a count.
void expect_same_item(const std::string& key, const ItemSnapshot& before, const ItemParams& after)
{
	EXPECT_EQ(before.name, after.name) << key << ".name";
	EXPECT_EQ(before.category, after.category) << key << ".category";
	EXPECT_EQ(before.params.color, after.color) << key << ".color";
	EXPECT_EQ(before.params.itemClass, after.itemClass) << key << ".itemClass";
	EXPECT_EQ(before.params.value, after.value) << key << ".value";
	EXPECT_EQ(before.params.pickableType, after.pickableType) << key << ".pickableType";

	EXPECT_EQ(before.params.consumableAmount, after.consumableAmount) << key << ".consumableAmount";
	EXPECT_EQ(before.params.range, after.range) << key << ".range";
	EXPECT_EQ(before.params.damage, after.damage) << key << ".damage";
	EXPECT_EQ(before.params.confuseTurns, after.confuseTurns) << key << ".confuseTurns";
	EXPECT_EQ(before.params.duration, after.duration) << key << ".duration";

	EXPECT_EQ(before.params.effect, after.effect) << key << ".effect";
	EXPECT_EQ(before.params.effectBonus, after.effectBonus) << key << ".effectBonus";

	EXPECT_EQ(before.params.strBonus, after.strBonus) << key << ".strBonus";
	EXPECT_EQ(before.params.dexBonus, after.dexBonus) << key << ".dexBonus";
	EXPECT_EQ(before.params.conBonus, after.conBonus) << key << ".conBonus";
	EXPECT_EQ(before.params.intBonus, after.intBonus) << key << ".intBonus";
	EXPECT_EQ(before.params.wisBonus, after.wisBonus) << key << ".wisBonus";
	EXPECT_EQ(before.params.chaBonus, after.chaBonus) << key << ".chaBonus";
	EXPECT_EQ(before.params.isSetMode, after.isSetMode) << key << ".isSetMode";

	EXPECT_EQ(before.params.nutritionValue, after.nutritionValue) << key << ".nutritionValue";
	EXPECT_EQ(before.params.goldAmount, after.goldAmount) << key << ".goldAmount";
	EXPECT_EQ(before.params.acBonus, after.acBonus) << key << ".acBonus";

	EXPECT_EQ(before.params.ranged, after.ranged) << key << ".ranged";
	EXPECT_EQ(before.params.handRequirement, after.handRequirement) << key << ".handRequirement";
	EXPECT_EQ(before.params.weaponSize, after.weaponSize) << key << ".weaponSize";

	EXPECT_EQ(before.params.consumableEffect, after.consumableEffect) << key << ".consumableEffect";
	EXPECT_EQ(before.params.consumableBuffType, after.consumableBuffType) << key << ".consumableBuffType";
	EXPECT_EQ(before.params.targetMode, after.targetMode) << key << ".targetMode";
	EXPECT_EQ(before.params.scrollAnimation, after.scrollAnimation) << key << ".scrollAnimation";

	EXPECT_EQ(before.params.baseWeight, after.baseWeight) << key << ".baseWeight";
	EXPECT_EQ(before.params.levelMin, after.levelMin) << key << ".levelMin";
	EXPECT_EQ(before.params.levelMax, after.levelMax) << key << ".levelMax";
	EXPECT_FLOAT_EQ(before.params.levelScaling, after.levelScaling) << key << ".levelScaling";
}

TEST_F(ItemRoundTripTest, EveryPersistedFieldSurvivesASave)
{
	std::map<std::string, ItemSnapshot> before;
	for (const std::string& key : ItemCreator::get_all_keys())
	{
		const ItemParams& params = ItemCreator::get_params(key);
		before.emplace(key, ItemSnapshot{ std::string{ params.name }, std::string{ params.category }, params });
	}
	ASSERT_FALSE(before.empty()) << "no items loaded; the test would pass vacuously";

	ItemCreator::save(roundTrip.string());
	ItemCreator::load(roundTrip.string());

	for (const auto& [key, snapshot] : before)
	{
		expect_same_item(key, snapshot, ItemCreator::get_params(key));
	}
}

// A category is only consulted through create_random_of_category, which also requires a
// positive baseWeight - so an item with a category and no weight has configuration that
// can never fire, and the category is a lie about how the item is obtained.
//
// This is narrower than "every item is reachable", which cannot be checked here: an item
// may also be produced by an enhanced rule pool or by a hardcoded ItemCreator::create
// call, and the second is not visible from a test.
TEST_F(ItemRoundTripTest, ACategoryImpliesTheItemCanBeDrawnFromIt)
{
	for (const std::string& key : ItemCreator::get_all_keys())
	{
		const ItemParams& params = ItemCreator::get_params(key);
		if (params.category.empty())
		{
			continue;
		}

		EXPECT_GT(params.baseWeight, 0)
			<< key << " is in category '" << params.category
			<< "' but has no spawn weight, so that category can never draw it";
	}
}
