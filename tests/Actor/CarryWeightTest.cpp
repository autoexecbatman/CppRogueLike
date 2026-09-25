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

#include "src/BodyPlanRegistry.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "tests/mocks/MockGameContext.h"

class CarryWeightTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Ten is the average 3d6 roll. What that allows is CarryCapacityTest's claim;
		// here every case is written against the limit, whatever it is.
		carrier.set_strength(10);
	}

	// A bare item of the given weight; nothing else about it matters to the rule.
	static std::unique_ptr<Item> weighing(int weight)
	{
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "stone", 0 });
		item->enhancement.weight = weight;
		return item;
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	Creature carrier{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "carrier", 0 } };
};

TEST_F(CarryWeightTest, AnItemExactlyAtTheLimitFits)
{
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	const auto item = weighing(limit);

	EXPECT_TRUE(InventoryOperations::is_within_weight_limit(*item, carrier, *ctx.dataManager));
}

TEST_F(CarryWeightTest, AnItemOneOverTheLimitDoesNot)
{
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	const auto item = weighing(limit + 1);

	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(*item, carrier, *ctx.dataManager));
}

TEST_F(CarryWeightTest, WhatIsAlreadyCarriedCountsAgainstTheLimit)
{
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	ASSERT_TRUE(InventoryOperations::add_item(carrier.inventoryData, weighing(limit - 5)).has_value());

	EXPECT_TRUE(InventoryOperations::is_within_weight_limit(*weighing(5), carrier, *ctx.dataManager));
	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(*weighing(6), carrier, *ctx.dataManager))
		<< "the pack's own weight must count, not only the new item's";
}

TEST_F(CarryWeightTest, TheCheckedAddRefusesAnItemOverTheLimit)
{
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);

	const auto refused = InventoryOperations::add_item_to_inventory(
		carrier.inventoryData,
		weighing(limit + 1),
		carrier,
		*ctx.dataManager);

	ASSERT_FALSE(refused.has_value());
	EXPECT_EQ(refused.error(), InventoryError::CAPACITY_EXCEEDED);
	EXPECT_TRUE(carrier.inventoryData.items.empty()) << "a refused item must not be kept";
}

// The book totals "the pounds of gear carried by the creature or character" and draws
// no line between the pack and the body (Player's Handbook, PDF page 160). Worn plate
// is still plate: putting it on cannot be a way to carry it for nothing.
TEST_F(CarryWeightTest, WhatIsWornCountsAgainstTheLimit)
{
	carrier.set_body_plan(mock.body_plans.get("humanoid"));
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	carrier.wear(weighing(limit + 1), EquipmentSlot::BODY);

	EXPECT_TRUE(InventoryOperations::is_overloaded(carrier, *ctx.dataManager))
		<< "armour heavier than the carrier weighed nothing once it was worn";
	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(*weighing(1), carrier, *ctx.dataManager))
		<< "a creature already over its limit in worn gear could still fill its pack";
}

// Pack and body add up. Half the limit worn and an empty pack leaves room for the
// other half exactly, and for nothing past it.
TEST_F(CarryWeightTest, ThePackAndTheBodyAreOneLoad)
{
	carrier.set_body_plan(mock.body_plans.get("humanoid"));
	const int limit = InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	const int worn = limit / 2;
	carrier.wear(weighing(worn), EquipmentSlot::BODY);

	EXPECT_TRUE(InventoryOperations::is_within_weight_limit(*weighing(limit - worn), carrier, *ctx.dataManager));
	EXPECT_FALSE(InventoryOperations::is_within_weight_limit(*weighing(limit - worn + 1), carrier, *ctx.dataManager))
		<< "the worn half of the load did not count";
}
