// file: ItemWeightTest.cpp
// What the things in items.json weigh, in pounds, against the book that prints it.
//
// The defect this pins: every record's `weight` had been copied from `baseWeight`,
// which is how often the item spawns, so a plate mail weighed 1 pound and a health
// potion 50. Weight is not decoration - InventoryOperations::get_total_weight sums it
// and the Strength row's Max. Carried Weight gates every pickup, purchase and unequip
// against the sum.
//
// Player's Handbook Table 44 (PDF pages 144-146) prints a weight for every weapon,
// every suit of armour, every shield and a handful of tools. Its two footnotes give
// the rest: `*` is "these items weigh little individually. Ten of these weigh one
// pound", and `**` is "no appreciable weight and should not be considered for
// encumbrance unless hundreds are carried". The game weighs in whole pounds, so both
// footnote classes are 0 here, which is the book's own instruction rather than a
// rounding choice.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemWeightTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "src/ItemClassification.h"
#include "src/ItemCreator.h"
#include "src/ItemRegistry.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
struct BookWeight
{
	std::string_view key;
	int pounds{ 0 };
};

struct ClassWeight
{
	ItemClass itemClass{ ItemClass::UNKNOWN };
	int pounds{ 0 };
};

// Every row the Player's Handbook prints a number for, read off Table 44.
constexpr std::array<BookWeight, 35> TABLE_FORTY_FOUR{ {
	{ "banded_mail", 35 },
	{ "brigandine", 35 },
	{ "chain_mail", 40 },
	{ "field_plate", 60 },
	{ "full_plate", 70 },
	{ "hide_armor", 30 },
	{ "leather_armor", 15 },
	{ "padded_armor", 10 },
	{ "plate_mail", 50 },
	{ "ring_mail", 30 },
	{ "scale_mail", 40 },
	{ "splint_mail", 40 },
	{ "studded_leather", 25 },
	{ "large_shield", 15 },
	{ "medium_shield", 10 },
	{ "small_shield", 5 },
	{ "battle_axe", 7 },
	{ "hand_axe", 5 },
	{ "dagger", 1 },
	{ "long_bow", 3 },
	{ "short_bow", 2 },
	{ "composite_bow", 3 },
	{ "heavy_crossbow", 14 },
	{ "light_crossbow", 7 },
	{ "club", 3 },
	{ "flail", 15 },
	{ "mace", 10 },
	{ "morning_star", 12 },
	{ "war_hammer", 6 },
	{ "bastard_sword", 10 },
	{ "long_sword", 4 },
	{ "scimitar", 4 },
	{ "short_sword", 3 },
	{ "two_handed_sword", 15 },
	{ "quarterstaff", 4 },
} };

// Tools the book weighs under Miscellaneous Equipment rather than under Weapons.
constexpr std::array<BookWeight, 3> MISCELLANEOUS_EQUIPMENT{ {
	{ "torch", 1 },
	{ "lockpick", 1 },
	{ "rope", 20 },
} };

// Weapons the book has no row for, each given the weight of the nearest row it does
// print. The owner's standing ruling is to match invented content to the closest book
// entry and decide it here rather than ask.
constexpr std::array<BookWeight, 4> MATCHED_TO_THE_NEAREST_ROW{ {
	// A greatsword and a great axe are the two-handed sword's 15.
	{ "great_sword", 15 },
	{ "great_axe", 15 },
	// A rapier is a one-handed sword, which the book weighs at 4 throughout.
	{ "rapier", 4 },
	// The generic staff is a quarterstaff.
	{ "staff", 4 },
} };

// Whole classes the book leaves out, where one answer covers every member and a new
// member should inherit it rather than arrive weightless.
constexpr std::array<ClassWeight, 5> MATCHED_BY_CLASS{ {
	// A magic helm is a helm: the basinet at 5, rather than the great helm's 10.
	{ ItemClass::HELMET, 5 },
	// A potion is a vial of liquid, which the book weighs as a flask of oil.
	{ ItemClass::POTION, 1 },
	// Cloaks, boots and gloves share this class. The book prices clothing and weighs
	// none of it, allowing five pounds for a worn outfit; one piece is a pound of that.
	{ ItemClass::GAUNTLETS, 1 },
	{ ItemClass::GIRDLE, 1 },
	// A meal, by the same reading of what the book leaves unweighed.
	{ ItemClass::FOOD, 1 },
} };

// The book's own weightless classes: jewellery and coins under `*`, parchment under
// `**`. Anything else reading 0 is an item nobody weighed.
constexpr std::array<std::string_view, 24> WEIGHS_NOTHING{ {
	"identify_scroll",
	"scroll_confusion",
	"scroll_fireball",
	"scroll_hold_person",
	"scroll_lightning",
	"scroll_sleep",
	"scroll_teleport",
	"ring_of_cold_resistance",
	"ring_of_fire_resistance",
	"ring_of_free_action",
	"ring_of_invisibility",
	"ring_of_protection_plus_1",
	"ring_of_protection_plus_2",
	"ring_of_regeneration",
	"ring_of_spell_storing",
	"amulet_of_health",
	"amulet_of_ogre_power",
	"amulet_of_protection",
	"amulet_of_wisdom",
	"amulet_of_yendor",
	"gem",
	"dungeon_key",
	"gold_coin",
	"sling",
} };
} // namespace

class ItemWeightTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
};

// Armour, shields and weapons, straight off the table.
TEST_F(ItemWeightTest, EveryWeightTheBookPrintsIsTheWeightTheDataCarries)
{
	for (const BookWeight& expected : TABLE_FORTY_FOUR)
	{
		EXPECT_EQ(mock.itemRegistry.get_params(expected.key).weight, expected.pounds) << expected.key;
	}
}

TEST_F(ItemWeightTest, TheToolsTheBookWeighsCarryItsNumbers)
{
	for (const BookWeight& expected : MISCELLANEOUS_EQUIPMENT)
	{
		EXPECT_EQ(mock.itemRegistry.get_params(expected.key).weight, expected.pounds) << expected.key;
	}
}

TEST_F(ItemWeightTest, AWeaponTheBookLacksWeighsWhatTheNearestRowDoes)
{
	for (const BookWeight& expected : MATCHED_TO_THE_NEAREST_ROW)
	{
		EXPECT_EQ(mock.itemRegistry.get_params(expected.key).weight, expected.pounds) << expected.key;
	}
}

// Every member of a class the book leaves out, so a new helm or potion cannot arrive
// with a weight of its own.
TEST_F(ItemWeightTest, AClassTheBookLeavesOutWeighsOneAnswerThroughout)
{
	for (const std::string& key : mock.itemRegistry.get_all_keys())
	{
		const ItemParams& params = mock.itemRegistry.get_params(key);
		const auto matches_class = [&params](const ClassWeight& entry)
		{
			return entry.itemClass == params.itemClass;
		};
		const auto found = std::ranges::find_if(MATCHED_BY_CLASS, matches_class);
		if (found == MATCHED_BY_CLASS.end())
		{
			continue;
		}

		EXPECT_EQ(params.weight, found->pounds) << key;
	}
}

// The census that catches an item nobody weighed: a weight of 0 is a claim that the
// item is one of the book's negligible ones, and only those may make it.
TEST_F(ItemWeightTest, OnlyTheBooksNegligibleItemsWeighNothing)
{
	for (const std::string& key : mock.itemRegistry.get_all_keys())
	{
		const int pounds = mock.itemRegistry.get_params(key).weight;
		EXPECT_GE(pounds, 0) << key;

		if (pounds > 0)
		{
			continue;
		}

		const bool negligible = std::ranges::find(WEIGHS_NOTHING, key) != WEIGHS_NOTHING.end();
		EXPECT_TRUE(negligible) << key << " weighs nothing and is not one of the book's negligible items";
	}
}

// Weight and spawn rarity are different questions about an item, and the whole defect
// was one answer serving both. A plate mail is rare and heavy; a health potion is
// common and light, and the two numbers must be free to say so.
TEST_F(ItemWeightTest, WeightIsNotTheSpawnRarity)
{
	const ItemParams& plateMail = mock.itemRegistry.get_params("plate_mail");
	const ItemParams& healthPotion = mock.itemRegistry.get_params("health_potion");

	EXPECT_GT(plateMail.weight, healthPotion.weight) << "plate mail must outweigh a potion";
	EXPECT_LT(plateMail.baseWeight, healthPotion.baseWeight) << "a potion must be commoner than plate mail";
}

// The data has to reach the item, or the table above is a description of a file
// nothing reads. A created suit of plate carries the pounds its record gives it.
TEST_F(ItemWeightTest, ACreatedItemCarriesItsRecordsWeight)
{
	GameContext ctx = mock.to_game_context();
	const auto plateMail = ItemCreator::create("plate_mail", Vector2D{ 0, 0 }, ctx);
	const auto dagger = ItemCreator::create("dagger", Vector2D{ 0, 0 }, ctx);

	ASSERT_TRUE(plateMail);
	ASSERT_TRUE(dagger);
	EXPECT_EQ(plateMail->enhancement.weight, 50);
	EXPECT_EQ(dagger->enhancement.weight, 1);
}
