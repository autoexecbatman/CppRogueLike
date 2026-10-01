// file: InventoryPersistenceTest.cpp
//
// What an inventory writes to a save, and what comes back.
//
// save_inventory and load_inventory are a pair, and the property that matters is
// that they agree on how many items there are. Three subsystems depend on it:
// Creature::save writes every creature's pack, GameStateManager writes the floor,
// and ShopKeeper writes the shop stock. An item lost between the two is lost from
// the game with nothing printed.
//
// load_inventory already refused to lose items - a broken record throws rather than
// handing back the ones before it. save_inventory did not: it skipped a null entry
// and wrote a record shorter than the inventory. The count tests below are what pin
// that, and AssertProbeDeathTest.SavingAnInventoryHoldingANullAborts is what proves
// the fault aborts rather than passing quietly.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=InventoryPersistenceTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/InventoryData.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "tests/mocks/MockGameContext.h"

using json = nlohmann::json;

namespace
{
constexpr size_t PACK_CAPACITY = 50;
constexpr Vector2D SOMEWHERE{ 3, 4 };
} // namespace

class InventoryPersistenceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// Puts one item of the given kind in the pack and reports whether it went in.
	bool stock(CreatureInventory& pack, std::string_view key)
	{
		auto item = ItemCreator::create(key, SOMEWHERE, ctx);
		return InventoryOperations::add_item(pack, std::move(item)).has_value();
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// The property the three callers depend on: the record holds every item, not most.
TEST_F(InventoryPersistenceTest, EveryItemReachesTheRecord)
{
	CreatureInventory pack{ PACK_CAPACITY };
	ASSERT_TRUE(stock(pack, "health_potion"));
	ASSERT_TRUE(stock(pack, "health_potion"));
	ASSERT_TRUE(stock(pack, "scroll_lightning"));

	json record;
	InventoryOperations::save_inventory(pack, record);

	EXPECT_EQ(record["inventory"].size(), pack.items.size())
		<< "the record holds a different number of items than the inventory it was written from";
	EXPECT_EQ(record["capacity"], PACK_CAPACITY);
}

// And the other direction, which is the one a player sees: what was saved comes back.
TEST_F(InventoryPersistenceTest, ARoundTripReturnsEveryItem)
{
	CreatureInventory original{ PACK_CAPACITY };
	ASSERT_TRUE(stock(original, "health_potion"));
	ASSERT_TRUE(stock(original, "scroll_lightning"));
	const size_t saved = original.items.size();

	json record;
	InventoryOperations::save_inventory(original, record);

	CreatureInventory restored{ 1 };
	InventoryOperations::load_inventory(restored, record);

	EXPECT_EQ(restored.items.size(), saved) << "the load returned a different number of items than the save wrote";
	EXPECT_EQ(restored.capacity, PACK_CAPACITY) << "the capacity did not survive the round trip";
}

// An empty inventory is a legal state, not a missing one - a creature that carries
// nothing still writes a record, and it has to come back empty rather than absent.
TEST_F(InventoryPersistenceTest, AnEmptyInventoryRoundTrips)
{
	CreatureInventory empty{ PACK_CAPACITY };

	json record;
	InventoryOperations::save_inventory(empty, record);

	CreatureInventory restored{ 1 };
	InventoryOperations::load_inventory(restored, record);

	EXPECT_TRUE(record["inventory"].is_array()) << "an empty inventory wrote something other than an empty array";
	EXPECT_EQ(restored.items.size(), 0u);
	EXPECT_EQ(restored.capacity, PACK_CAPACITY);
}
