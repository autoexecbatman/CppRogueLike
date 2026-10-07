// file: FloorStackPickupTest.cpp
//
// Picking up from a tile that holds more than one item: the player chooses which.
//
// What it is for. pick_item took the first item it found standing on the player's
// tile and ignored the rest, so a heap came up one arbitrary item per keypress and
// the player had no say in the order. A tile holding one item still picks up with no
// menu in the way; a tile holding several offers the choice.
//
// Tried and rejected: a case pinning one menu entry per item on the tile. ListMenu
// keeps its entries private and BaseMenu keeps menuHeight protected, so the entry
// count has no observable consequence here and an empty menu reads the same as a full
// one. The mutation that empties it is left out of the plan rather than left standing
// as a survivor. Driving the menu with keys is what would catch it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=FloorStackPickupTest.*

#include <gtest/gtest.h>

#include <deque>
#include <memory>

#include "src/BaseMenu.h"
#include "src/Colors.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/Pickup.h"
#include "src/Player.h"
#include "src/Renderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int STRONG_ENOUGH_TO_CARRY_ANYTHING = 18;
const Vector2D UNDERFOOT{ 5, 5 };
const Vector2D ELSEWHERE{ 9, 9 };
} // namespace

class FloorStackPickupTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		player->position = UNDERFOOT;
		player->set_strength(STRONG_ENOUGH_TO_CARRY_ANYTHING);

		ctx.playerOwner = &player;
		ctx.menus = &menus;
		// ListMenu asks the renderer how wide its text is. No window is open here, so
		// measure_text falls through to raylib's default font and answers zero, which is
		// a menu of no width - enough to build one and count its entries.
		ctx.renderer = &renderer;
		// BaseMenu::menu_new asserts these three. tileConfig the mock already wires;
		// the other two exist only to be non-null, since nothing here reads a key.
		ctx.inputSystem = &input;
	}

	// A light item of the given name standing on the given tile.
	Item& drop(const std::string& name, Vector2D where)
	{
		auto item = std::make_unique<Item>(where, ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
		item->enhancement.weight = 1;
		Item& placed = *item;
		const auto added = InventoryOperations::add_item(*ctx.floorInventory, std::move(item));
		EXPECT_TRUE(added.has_value()) << "the fixture could not put " << name << " on the floor";
		return placed;
	}

	size_t items_underfoot() const
	{
		return InventoryOperations::items_at(*ctx.floorInventory, UNDERFOOT).size();
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	std::deque<std::unique_ptr<BaseMenu>> menus{};
	Renderer renderer{};
	InputSystem input{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(UNDERFOOT) };
};

TEST_F(FloorStackPickupTest, ItemsAtAnswersOnlyTheTileAskedFor)
{
	drop("dagger", UNDERFOOT);
	drop("rope", UNDERFOOT);
	drop("shield", ELSEWHERE);

	EXPECT_EQ(InventoryOperations::items_at(*ctx.floorInventory, UNDERFOOT).size(), 2u);
	EXPECT_EQ(InventoryOperations::items_at(*ctx.floorInventory, ELSEWHERE).size(), 1u);
}

TEST_F(FloorStackPickupTest, ItemsAtABareTileIsEmpty)
{
	EXPECT_TRUE(InventoryOperations::items_at(*ctx.floorInventory, UNDERFOOT).empty());
}

TEST_F(FloorStackPickupTest, OneItemIsTakenWithNoMenuInTheWay)
{
	drop("dagger", UNDERFOOT);

	Pickup::from_floor(*player, ctx);

	EXPECT_TRUE(menus.empty()) << "a single item made the player answer a menu to take it";
	EXPECT_EQ(items_underfoot(), 0u);
	EXPECT_EQ(InventoryOperations::get_item_count(player->inventoryData), 1u);
}

TEST_F(FloorStackPickupTest, AStackOpensAMenuAndTakesNothingYet)
{
	drop("dagger", UNDERFOOT);
	drop("rope", UNDERFOOT);

	Pickup::from_floor(*player, ctx);

	EXPECT_EQ(menus.size(), 1u) << "a heap was picked through without ever asking";
	EXPECT_EQ(items_underfoot(), 2u) << "something was taken before the player chose";
	EXPECT_EQ(InventoryOperations::get_item_count(player->inventoryData), 0u);
}

TEST_F(FloorStackPickupTest, TakingOneOfAStackLeavesTheRestOnTheFloor)
{
	drop("dagger", UNDERFOOT);
	Item& rope = drop("rope", UNDERFOOT);

	Pickup::take_floor_item(*player, rope, ctx);

	EXPECT_EQ(items_underfoot(), 1u);
	EXPECT_EQ(InventoryOperations::items_at(*ctx.floorInventory, UNDERFOOT).front()->actorData.name, "dagger");
	EXPECT_EQ(InventoryOperations::get_item_count(player->inventoryData), 1u);
}

TEST_F(FloorStackPickupTest, AnIdOutlivesThePointerTheMenuCouldNotHold)
{
	Item& rope = drop("rope", UNDERFOOT);
	const UniqueId::IdType ropeId = rope.uniqueId;

	EXPECT_EQ(InventoryOperations::find_item_by_id(*ctx.floorInventory, ropeId), &rope);

	Pickup::take_floor_item(*player, rope, ctx);

	EXPECT_EQ(InventoryOperations::find_item_by_id(*ctx.floorInventory, ropeId), nullptr)
		<< "the floor still answers for an item somebody already carried off";
}
