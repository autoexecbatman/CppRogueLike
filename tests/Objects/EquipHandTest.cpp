// file: EquipHandTest.cpp
// Which item classes a hand will take.
//
// The rule, in words: the right hand takes a weapon or a shield. The left takes
// a shield, or a weapon light enough to be swung in one hand - a two-handed
// weapon needs both, so it cannot sit in the off-hand while the other holds
// something else. Nothing that is not a weapon or a shield goes in either: a
// potion is drunk, a ring is worn, gold is carried.
//
// This existed once, in src/ItemClassification.cpp.test.cpp, which was in no
// CMakeLists and so had never run. That file was deleted; the claim it made is
// the only one of its twelve worth keeping, so it is made here as a test that
// does run.
//
// The four groups below partition ALL_ITEM_CLASS, and the test asserts that they
// do - so an item class added to the enum without a decision about hands fails
// here rather than defaulting quietly.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EquipHandTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include "src/ItemClassification.h"

namespace
{
// Named here so each assertion reads as the claim it makes.
bool right_hand_takes(ItemClass itemClass)
{
	return ItemClassificationUtils::can_equip_to_right_hand(itemClass);
}

bool left_hand_takes(ItemClass itemClass)
{
	return ItemClassificationUtils::can_equip_to_left_hand(itemClass);
}

// AXE is here because Table 44 gives a battle axe Size M at 7 lb: the book marks a
// two-handed weapon with a "Two-handed" sub-row or Size L, and neither axe has one.
constexpr std::array ONE_HANDED_WEAPONS{
	ItemClass::DAGGER,
	ItemClass::SWORD,
	ItemClass::HAMMER,
	ItemClass::MACE,
	ItemClass::SLING,
	ItemClass::AXE,
};

constexpr std::array TWO_HANDED_WEAPONS{
	ItemClass::GREAT_SWORD,
	ItemClass::STAFF,
	ItemClass::BOW,
	ItemClass::CROSSBOW,
};

constexpr std::array SHIELDS{ ItemClass::SHIELD };

constexpr std::array NOT_HELD_IN_A_HAND{
	ItemClass::UNKNOWN,
	ItemClass::ARMOR,
	ItemClass::HELMET,
	ItemClass::RING,
	ItemClass::AMULET,
	ItemClass::GAUNTLETS,
	ItemClass::GIRDLE,
	ItemClass::POTION,
	ItemClass::SCROLL,
	ItemClass::FOOD,
	ItemClass::GOLD_COIN,
	ItemClass::GEM,
	ItemClass::TOOL,
	ItemClass::QUEST_ITEM,
};
} // namespace

TEST(EquipHandTest, TheFourGroupsAreTheWholeEnum)
{
	std::vector<ItemClass> grouped;
	grouped.insert(grouped.end(), ONE_HANDED_WEAPONS.begin(), ONE_HANDED_WEAPONS.end());
	grouped.insert(grouped.end(), TWO_HANDED_WEAPONS.begin(), TWO_HANDED_WEAPONS.end());
	grouped.insert(grouped.end(), SHIELDS.begin(), SHIELDS.end());
	grouped.insert(grouped.end(), NOT_HELD_IN_A_HAND.begin(), NOT_HELD_IN_A_HAND.end());

	ASSERT_EQ(grouped.size(), ALL_ITEM_CLASS.size()) << "an item class was added and no hand rule was decided for it";

	for (const ItemClass itemClass : ALL_ITEM_CLASS)
	{
		EXPECT_NE(std::ranges::find(grouped, itemClass), grouped.end())
			<< "an item class of the enum appears in none of the four groups";
	}
}

TEST(EquipHandTest, AOneHandedWeaponGoesInEitherHand)
{
	for (const ItemClass weapon : ONE_HANDED_WEAPONS)
	{
		EXPECT_TRUE(right_hand_takes(weapon));
		EXPECT_TRUE(left_hand_takes(weapon));
	}
}

TEST(EquipHandTest, ATwoHandedWeaponGoesInTheMainHandOnly)
{
	for (const ItemClass weapon : TWO_HANDED_WEAPONS)
	{
		EXPECT_TRUE(right_hand_takes(weapon));
		EXPECT_FALSE(left_hand_takes(weapon)) << "a two-handed weapon was allowed in the off-hand";
	}
}

TEST(EquipHandTest, AShieldGoesInEitherHand)
{
	for (const ItemClass shield : SHIELDS)
	{
		EXPECT_TRUE(right_hand_takes(shield));
		EXPECT_TRUE(left_hand_takes(shield));
	}
}

TEST(EquipHandTest, WhatIsNotAWeaponOrAShieldGoesInNeitherHand)
{
	for (const ItemClass itemClass : NOT_HELD_IN_A_HAND)
	{
		EXPECT_FALSE(right_hand_takes(itemClass));
		EXPECT_FALSE(left_hand_takes(itemClass));
	}
}

// The off-hand's rule is the main hand's plus a restriction, never a different set.
TEST(EquipHandTest, WhateverTheOffHandTakesTheMainHandTakesToo)
{
	for (const ItemClass itemClass : ALL_ITEM_CLASS)
	{
		if (left_hand_takes(itemClass))
		{
			EXPECT_TRUE(right_hand_takes(itemClass))
				<< "the off-hand accepts something the main hand refuses";
		}
	}
}

// end of file: EquipHandTest.cpp
