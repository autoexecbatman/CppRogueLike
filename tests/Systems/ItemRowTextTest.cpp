// file: ItemRowTextTest.cpp
//
// What an item's row says about its worth and its weight.
//
// What it is for. The equipment screen and the backpack list describe the same items and
// each wrote its own trailing bracket. When weight was added to the equipment screen the
// backpack kept saying only a price, so the two screens disagreed about what an item is.
// They read one function now.
//
// Weight is on every row because the band at the top of the screen measures a total
// against a carrying limit, and a row with no weight cannot be checked against it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemRowTextTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Colors.h"
#include "src/InventoryUI.h"
#include "src/Item.h"

namespace
{
std::unique_ptr<Item> an_item(int value, int weight)
{
	auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "thing", ColorPairId::WHITE_BLACK });
	item->set_value(value);
	item->enhancement.weight = weight;
	return item;
}
} // namespace

TEST(ItemRowTextTest, SomethingWorthBuyingShowsBothNumbers)
{
	EXPECT_EQ(InventoryActions::value_and_weight(*an_item(5, 15)), " (5 gp, 15 lb)");
}

TEST(ItemRowTextTest, SomethingWorthlessStillShowsItsWeight)
{
	// A corpse is worth nothing and weighs a great deal, and it is exactly the thing a
	// player needs the weight of.
	EXPECT_EQ(InventoryActions::value_and_weight(*an_item(0, 8)), " (8 lb)");
}

TEST(ItemRowTextTest, SomethingWeightlessStillSaysSo)
{
	// Zero is an answer. A blank reads as a row that failed to say.
	EXPECT_EQ(InventoryActions::value_and_weight(*an_item(2, 0)), " (2 gp, 0 lb)");
}

TEST(ItemRowTextTest, TheTextBeginsWithASpaceSoItAppends)
{
	// Callers append it to a line they have already built; without the leading space it
	// runs into the item's name.
	const auto text = InventoryActions::value_and_weight(*an_item(1, 1));

	ASSERT_FALSE(text.empty());
	EXPECT_EQ(text.front(), ' ');
}
