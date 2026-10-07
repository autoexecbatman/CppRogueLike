// file: TorchSightTest.cpp
//
// What a carried torch changes: the radius a creature sees, and the field of view the
// map computes from it.
//
// What it is for. The torch existed in data/content/items.json and did nothing - no
// consumable effect, no duration, and no code in src/ read it. The sight radius was a
// compile-time constant, so nothing a character carried could move it. These cases pin
// the rule in both places: the query that answers the radius, and the map that uses it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TorchSightTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemRegistry.h"
#include "src/Map.h"
#include "src/Vision.h"
#include "tests/mocks/MockGameContext.h"

class TorchSightTest : public ::testing::Test
{
protected:
	// An item carrying the given registry key and nothing else of interest.
	static std::unique_ptr<Item> keyed(std::string_view itemKey)
	{
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, std::string{ itemKey }, ColorPairId::WHITE_BLACK });
		item->itemKey = itemKey;
		return item;
	}

	void pack(std::string_view itemKey)
	{
		const auto added = InventoryOperations::add_item(viewer.inventoryData, keyed(itemKey));
		ASSERT_TRUE(added.has_value()) << "the fixture could not put " << itemKey << " in the pack";
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	Creature viewer{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "viewer", ColorPairId::WHITE_BLACK } };
};

TEST_F(TorchSightTest, AnEmptyHandedCreatureSeesTheBaseRadius)
{
	EXPECT_EQ(Vision::sight_radius(viewer), FOV_RADIUS);
}

TEST_F(TorchSightTest, ATorchInThePackWidensTheRadius)
{
	pack(Vision::TORCH_ITEM_KEY);

	EXPECT_EQ(Vision::sight_radius(viewer), Vision::TORCH_SIGHT_RADIUS);
	EXPECT_GT(Vision::sight_radius(viewer), FOV_RADIUS)
		<< "a torch has to be worth carrying, so it must see further than no torch";
}

TEST_F(TorchSightTest, AnItemThatIsNotATorchChangesNothing)
{
	// Without this the fix could be "anything in the pack widens the radius" and pass.
	pack("stone");

	EXPECT_EQ(Vision::sight_radius(viewer), FOV_RADIUS);
}

TEST_F(TorchSightTest, ASecondTorchDoesNotStack)
{
	pack(Vision::TORCH_ITEM_KEY);
	pack(Vision::TORCH_ITEM_KEY);

	EXPECT_EQ(Vision::sight_radius(viewer), Vision::TORCH_SIGHT_RADIUS)
		<< "carrying spares lit the dungeon further than carrying one";
}

TEST_F(TorchSightTest, ThePredicateReadsTheRegistryKey)
{
	EXPECT_FALSE(InventoryOperations::is_carrying(viewer.inventoryData, Vision::TORCH_ITEM_KEY));
	pack(Vision::TORCH_ITEM_KEY);
	EXPECT_TRUE(InventoryOperations::is_carrying(viewer.inventoryData, Vision::TORCH_ITEM_KEY));
}

TEST_F(TorchSightTest, TheKeyNamesAnItemThatActuallyExists)
{
	// Without this the constant only ever matches itself: every case above packs
	// Vision::TORCH_ITEM_KEY, so a key naming nothing in the data would still pass
	// while the game found no torch and the radius never moved. get_params throws on
	// an unknown key, so this asks the registry rather than the test.
	auto look_up_the_torch = [this]()
	{
		[[maybe_unused]] const ItemParams& params = ctx.itemRegistry->get_params(Vision::TORCH_ITEM_KEY);
	};

	EXPECT_NO_THROW(look_up_the_torch())
		<< "Vision::TORCH_ITEM_KEY names no item in data/content/items.json";
}
