// Checks the strength-based carry limit: what fits, what does not, and that the checked
// add refuses without keeping what it refused.
//
// What it is for. The limit was written out in three places - inside
// add_item_to_inventory, in the pickup pre-check, and nowhere at all in the shop, which
// sold what could not be lifted. It is one query now, is_within_weight_limit, and this
// file pins the query and the add that relies on it. Weights are set on the item rather
// than read from the data, so each case is about the rule.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CarryWeightTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"

class CarryWeightTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Ten is the average 3d6 roll, a limit of 50.
		carrier.set_strength(10);
	}

	// A bare item of the given weight; nothing else about it matters to the rule.
	static std::unique_ptr<Item> weighing(int weight)
	{
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "stone", 0 });
		item->enhancement.weight = weight;
		return item;
	}

	Creature carrier{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "carrier", 0 } };
};

TEST_F(CarryWeightTest, AnItemExactlyAtTheLimitFits)
{
	const int limit = InventoryOperations::get_max_weight(carrier);
	const auto item = weighing(limit);

	EXPECT_TRUE(InventoryOperations::is_within_weight_limit(carrier.inventoryData, *item, carrier));
}

TEST_F(CarryWeightTest, AnItemOneOverTheLimitDoesNot)
{
	const int limit = InventoryOperations::get_max_weight(carrier);
	const auto item = weighing(limit + 1);

	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(carrier.inventoryData, *item, carrier));
}

TEST_F(CarryWeightTest, WhatIsAlreadyCarriedCountsAgainstTheLimit)
{
	const int limit = InventoryOperations::get_max_weight(carrier);
	ASSERT_TRUE(InventoryOperations::add_item(carrier.inventoryData, weighing(limit - 5)).has_value());

	EXPECT_TRUE(InventoryOperations::is_within_weight_limit(carrier.inventoryData, *weighing(5), carrier));
	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(carrier.inventoryData, *weighing(6), carrier))
		<< "the pack's own weight must count, not only the new item's";
}

TEST_F(CarryWeightTest, TheCheckedAddRefusesAnItemOverTheLimit)
{
	const int limit = InventoryOperations::get_max_weight(carrier);

	const auto refused = InventoryOperations::add_item_to_inventory(
		carrier.inventoryData,
		weighing(limit + 1),
		carrier);

	ASSERT_FALSE(refused.has_value());
	EXPECT_EQ(refused.error(), InventoryError::CAPACITY_EXCEEDED);
	EXPECT_TRUE(carrier.inventoryData.items.empty()) << "a refused item must not be kept";
}
