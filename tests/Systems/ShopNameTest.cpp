// file: ShopNameTest.cpp
// The shop's name is drawn from the game's dice.
//
// It used to come from rand(), which nothing calls srand for, so every
// playthrough named its shops in the same order and no test could ask for a
// particular one. The name is the only thing a player sees of that draw.
//
// A shop keeps five lists of eight names, one per type, and the constructor
// picks from the list its type names. So one roll decides the name, and the same
// roll has to give the same name twice.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ShopNameTest.*

#include <gtest/gtest.h>

#include <string>

#include "src/RandomDice.h"
#include "src/ShopKeeper.h"

namespace
{
// Eight names per list, so this is the last of them.
constexpr int LAST_NAME_INDEX = 7;
constexpr int FIRST_NAME_INDEX = 0;
} // namespace

class ShopNameTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		dice.set_test_mode(false);
		dice.clear_fixed_rolls();
	}

	std::string name_drawn_at(int index, ShopType type)
	{
		dice.set_test_mode(true);
		dice.set_next_roll(index);
		return ShopKeeper{ type, ShopQuality::AVERAGE, dice }.get_shop_name();
	}

	RandomDice dice;
};

// The roll is what decides, so the same roll decides the same way.
TEST_F(ShopNameTest, TheSameRollNamesTheSameShop)
{
	const std::string first = name_drawn_at(3, ShopType::WEAPON_SHOP);
	const std::string again = name_drawn_at(3, ShopType::WEAPON_SHOP);

	EXPECT_FALSE(first.empty()) << "a shop was built without a name";
	EXPECT_EQ(first, again) << "the same roll gave two names, so the roll is not what decides";
}

// And a different roll has to reach a different name, or the first pair agreeing
// would show only that the list has one entry.
TEST_F(ShopNameTest, ADifferentRollNamesADifferentShop)
{
	EXPECT_NE(name_drawn_at(FIRST_NAME_INDEX, ShopType::WEAPON_SHOP),
		name_drawn_at(LAST_NAME_INDEX, ShopType::WEAPON_SHOP));
}

// Each type draws from its own list, so two types on the same roll differ.
TEST_F(ShopNameTest, EachTypeDrawsFromItsOwnList)
{
	EXPECT_NE(name_drawn_at(FIRST_NAME_INDEX, ShopType::WEAPON_SHOP),
		name_drawn_at(FIRST_NAME_INDEX, ShopType::ARMOR_SHOP));
	EXPECT_NE(name_drawn_at(FIRST_NAME_INDEX, ShopType::POTION_SHOP),
		name_drawn_at(FIRST_NAME_INDEX, ShopType::SCROLL_SHOP));
}

// end of file: ShopNameTest.cpp
