// Checks that a treasure-room gold pile is the same kind of item as a floor gold pile.
//
// What it is for. Gold was built three different ways. The floor path goes through
// ItemCreator::create_with_gold_amount, which sets the behaviour and the item's value
// from one number. generate_treasure hand-built its pile instead - Gold{amount} with no
// set_value - so the pile paid out the rolled amount while its value stayed at the
// default 1, and it carried no item key, no item class and a tile looked up from
// tile_config rather than from the item registry.
//
// What it deliberately does not check. Not how much gold a treasure room should hold,
// which is policy and differs from the floor rule on purpose; only that whatever it
// holds is worth what it pays.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TreasureGoldTest.*

#include "src/ItemFactory.h"
#include "src/ItemCreator.h"
#include "src/Item.h"
#include "src/Pickable.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

#include <variant>

class TreasureGoldTest : public ::testing::Test
{
protected:
	MockGameContext mock;
	Map map{ 40, 25 };
	MessageSystem messageSystem;
	GameContext ctx;

	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.map = &map;
		ctx.messageSystem = &messageSystem;
		map.init_tiles();
	}

	// The one item on the floor carrying a Gold behaviour.
	const Item* find_gold() const
	{
		for (const auto& item : ctx.floorInventory->items)
		{
			if (item && item->behavior && std::holds_alternative<Gold>(*item->behavior))
			{
				return item.get();
			}
		}
		return nullptr;
	}
};

TEST_F(TreasureGoldTest, ATreasurePileIsWorthWhatItPaysOut)
{
	ItemFactory::generate_treasure(Vector2D{ 10, 10 }, 1, 1, ctx);

	const Item* gold = find_gold();
	ASSERT_NE(gold, nullptr) << "generate_treasure always places gold";
	const int paidOut = std::get<Gold>(*gold->behavior).amount;
	ASSERT_GT(paidOut, 1) << "the rolled amount must exceed the default value for this to discriminate";

	EXPECT_EQ(gold->get_value(), paidOut)
		<< "the pile pays out " << paidOut << " but is valued at " << gold->get_value();
}

TEST_F(TreasureGoldTest, ATreasurePileIsTheSameItemAsAFloorPile)
{
	ItemFactory::generate_treasure(Vector2D{ 10, 10 }, 1, 1, ctx);

	const Item* gold = find_gold();
	ASSERT_NE(gold, nullptr);

	// A hand-built item has none of these, so each one says the pile came through the
	// registry rather than being assembled at the call site.
	EXPECT_EQ(gold->itemKey, "gold_coin") << "a pile with no key cannot be identified on load";
	EXPECT_EQ(gold->itemClass, mock.itemRegistry.get_params("gold_coin").itemClass);
	EXPECT_EQ(gold->actorData.name, std::string(mock.itemRegistry.get_params("gold_coin").name));
}
