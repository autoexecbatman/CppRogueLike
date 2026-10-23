// file: InventoryCloseAfterUseTest.cpp
// Which uses close the inventory. Reading a scroll of teleportation moves the player
// somewhere else, so the screen closes and the map shows where they landed; drinking a
// healing potion leaves the player where they were, and the screen stays open for the
// next choice. The owner's rule, 2026-10-17: "After using a teleportation scroll the
// inventory UI should close automatically."
//
// Driven the way a player drives it: the inventory pushed onto the menu stack, a key
// pressed, and MenuManager run for a frame, which is what removes a closed menu. The
// right-click path is driven too, because its "Use" runs from the row menu on top of the
// inventory rather than from the inventory's own key handling.
//
// Each test runs in a child process with tests/mocks/HiddenRenderer.h, and skips with
// the reason where there is no OpenGL driver, which is the CI runner today.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=InventoryCloseAfterUseTest.*

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include <raylib.h>

#include "src/Actor.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/InventoryUI.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/MenuManager.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/Renderer.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A 3 by 3 room of floor, so a teleport has somewhere to land other than where it began.
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_SIZE = 3;
constexpr Vector2D PLAYER_TILE{ ROOM_COL, ROOM_ROW };
// A creature built in a test has no Strength, and the pack is weight-gated by it.
constexpr int CARRIER_STRENGTH = 18;
// Wounded below the potion's ceiling, so drinking it heals and is a spent turn.
constexpr int MAX_HIT_POINTS = 20;
constexpr int WOUNDED_HIT_POINTS = 5;

// The mock's context with a renderer, a floored room and a wounded player carrying one
// item, and the inventory open on its backpack pane on the menu stack.
class OpenInventory
{
public:
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx{};

	explicit OpenInventory(const char* itemKey)
	{
		ctx = mock.to_game_context();
		ctx.renderer = &hidden.renderer;

		mock.map.init_tiles();
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				mock.map.set_tile(Vector2D{ column, row }, TileType::FLOOR);
			}
		}

		mock.player = std::make_unique<Player>(PLAYER_TILE);
		Player& player = *mock.player;
		player.healthPool = std::make_unique<HealthPool>(MAX_HIT_POINTS);
		player.armorClass = std::make_unique<ArmorClass>(10);
		player.experienceReward = std::make_unique<ExperienceReward>(0);
		player.set_creature_level(1);
		player.set_strength(CARRIER_STRENGTH);
		player.set_hp(WOUNDED_HIT_POINTS);

		exit_unless(
			InventoryOperations::add_item(
				player.inventoryData,
				ItemCreator::create(itemKey, PLAYER_TILE, ctx))
				.has_value(),
			"the item did not fit the pack");

		mock.menus.push_back(
			std::make_unique<InventoryUI>(
				player,
				InventoryScreen::BACKPACK,
				ctx));
	}

	// One frame of the menu stack with a key pressed, as the game loop runs it.
	void press(GameKey key)
	{
		ctx.inputSystem->queue_key(key, 0);
		run_one_frame();
	}

	// One pass of the menu stack inside a frame, which Game::tick opens and closes.
	void run_one_frame()
	{
		hidden.renderer.begin_frame();
		ctx.menuManager->handle_menus(*ctx.menus, ctx);
		hidden.renderer.end_frame();
	}
};

// The backpack pane's first content row is the category heading; the scroll is below it.
int scroll_row_y(const OpenInventory& open)
{
	return panel_text_row_y(
		0,
		open.hidden.renderer.get_tile_size(),
		FIRST_CONTENT_ROW + 1);
}

// Enter on the scroll of teleportation: the scroll is read, the turn is spent, and the
// inventory is off the menu stack.
[[noreturn]] void reading_the_teleport_scroll_closes_the_inventory()
{
	OpenInventory open{ "scroll_teleport" };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the scroll was not read");
	exit_unless(
		open.ctx.gameState->get_time() > timeBefore,
		"reading the scroll spent no turn");
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the teleport");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Enter on a healing potion: the potion is drunk and the turn is spent, and the
// inventory stays open.
[[noreturn]] void drinking_a_healing_potion_leaves_the_inventory_open()
{
	OpenInventory open{ "health_potion" };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the potion was not drunk");
	exit_unless(
		open.ctx.gameState->get_time() > timeBefore,
		"drinking the potion spent no turn");
	exit_unless(
		open.mock.menus.size() == 1,
		"the inventory closed after a healing potion");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Right-click on the scroll's row and choose Use: the row menu closes, and so does the
// inventory beneath it, on the next frame and without reading that frame's key.
[[noreturn]] void
using_the_teleport_scroll_from_the_row_menu_closes_the_inventory()
{
	OpenInventory open{ "scroll_teleport" };
	const int timeBefore = open.ctx.gameState->get_time();

	SetMousePosition(HIDDEN_WINDOW_WIDTH / 2, scroll_row_y(open) + 1);

	open.press(GameKey::MOUSE_RIGHT);
	exit_unless(
		open.mock.menus.size() == 2,
		"the right-click opened no row menu");

	// The row menu's first entry is Use.
	open.press(GameKey::ENTER);
	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the scroll was not read");
	exit_unless(
		open.ctx.gameState->get_time() > timeBefore,
		"reading the scroll spent no turn");

	// Enter on the frame after, with a potion now in the pack: the inventory closes
	// without acting on it, so the potion is still there.
	exit_unless(
		InventoryOperations::add_item(
			open.mock.player->inventoryData,
			ItemCreator::create("health_potion", PLAYER_TILE, open.ctx))
			.has_value(),
		"the potion did not fit the pack");
	open.press(GameKey::ENTER);
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the teleport");
	exit_unless(
		open.mock.player->inventoryData.items.size() == 1,
		"the closing inventory acted on the next frame's key");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// A left click on the scroll's row reads it, and the inventory closes.
[[noreturn]] void clicking_the_teleport_scroll_closes_the_inventory()
{
	OpenInventory open{ "scroll_teleport" };
	SetMousePosition(HIDDEN_WINDOW_WIDTH / 2, scroll_row_y(open) + 1);

	open.press(GameKey::MOUSE_LEFT);

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the click did not read the scroll");
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the teleport");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// The letter beside the scroll's row reads it, and the inventory closes.
[[noreturn]] void the_teleport_scroll_letter_closes_the_inventory()
{
	OpenInventory open{ "scroll_teleport" };

	// The scroll is the only item, so its letter is a.
	open.ctx.inputSystem->queue_key(GameKey::NONE, 'a');
	open.run_one_frame();

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the letter did not read the scroll");
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the teleport");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// A goblin beside the player with more hit points than any scroll deals, so a scroll aimed
// at the nearest creature has a target that survives it.
void place_a_goblin_beside_the_player(OpenInventory& open)
{
	constexpr int GOBLIN_HIT_POINTS = 200;
	auto goblin = std::make_unique<Creature>(
		Vector2D{ PLAYER_TILE.x + 1, PLAYER_TILE.y },
		ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
	goblin->healthPool = std::make_unique<HealthPool>(GOBLIN_HIT_POINTS);
	goblin->armorClass = std::make_unique<ArmorClass>(10);
	open.mock.creatures.push_back(std::move(goblin));
}

// Enter on a scroll that acts at once on what the reader sees: read, the turn spent, and
// the inventory off the menu stack. Sleep and hold person both work this way.
void reading_closes_the_inventory(const char* scrollKey)
{
	OpenInventory open{ scrollKey };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the scroll was not read");
	exit_unless(
		open.ctx.gameState->get_time() > timeBefore,
		"reading the scroll spent no turn");
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the scroll was read");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

[[noreturn]] void reading_the_sleep_scroll_closes_the_inventory()
{
	reading_closes_the_inventory("scroll_sleep");
	std::_Exit(HIDDEN_RENDERER_EXPECTATION_FAILED);
}

[[noreturn]] void reading_the_hold_person_scroll_closes_the_inventory()
{
	reading_closes_the_inventory("scroll_hold_person");
	std::_Exit(HIDDEN_RENDERER_EXPECTATION_FAILED);
}

// Lightning strikes the nearest creature: with a goblin beside the player the scroll is
// read and the inventory closes.
[[noreturn]] void lightning_with_a_target_closes_the_inventory()
{
	OpenInventory open{ "scroll_lightning" };
	place_a_goblin_beside_the_player(open);

	open.press(GameKey::ENTER);

	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the lightning scroll was not read");
	exit_unless(
		open.mock.menus.empty(),
		"the inventory is still open after the lightning struck");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// With nothing in range the lightning scroll is not read: it stays in the pack, no turn
// passes, and the inventory stays open for another choice.
[[noreturn]] void lightning_with_nothing_in_range_leaves_the_inventory_open()
{
	OpenInventory open{ "scroll_lightning" };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);

	exit_unless(
		open.mock.player->inventoryData.items.size() == 1,
		"the lightning scroll was spent on nothing");
	exit_unless(
		open.ctx.gameState->get_time() == timeBefore,
		"a scroll that was not read spent a turn");
	exit_unless(
		open.mock.menus.size() == 1,
		"the inventory closed though nothing was read");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Confusion is aimed at a tile: Enter opens the targeting cursor over the inventory, the
// cursor moves to the empty tile beside the player, and Enter there reads the scroll.
// When the cursor closes, the inventory goes with it rather than coming back.
[[noreturn]] void
aiming_the_confusion_scroll_closes_the_inventory_when_targeting_ends()
{
	OpenInventory open{ "scroll_confusion" };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);
	exit_unless(
		open.mock.menus.size() == 2,
		"Enter on the confusion scroll opened no targeting cursor");

	open.press(GameKey::RIGHT);
	open.press(GameKey::ENTER);
	exit_unless(
		open.mock.player->inventoryData.items.empty(),
		"the confusion scroll was not read");
	exit_unless(
		open.ctx.gameState->get_time() > timeBefore,
		"reading the confusion scroll spent no turn");

	// The frame after the cursor closes: the inventory beneath it closes without showing.
	open.press(GameKey::NONE);
	exit_unless(
		open.mock.menus.empty(),
		"the inventory came back after the scroll was aimed");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Fireball is aimed at a tile too, and cancelling the cursor keeps the scroll and the
// turn; the inventory still goes, because reading handed it over to the cursor.
[[noreturn]] void cancelling_the_fireball_cursor_closes_the_inventory()
{
	OpenInventory open{ "scroll_fireball" };
	const int timeBefore = open.ctx.gameState->get_time();

	open.press(GameKey::ENTER);
	exit_unless(
		open.mock.menus.size() == 2,
		"Enter on the fireball scroll opened no targeting cursor");

	open.press(GameKey::ESCAPE);
	exit_unless(
		open.mock.player->inventoryData.items.size() == 1,
		"a cancelled fireball spent the scroll");
	exit_unless(
		open.ctx.gameState->get_time() == timeBefore,
		"a cancelled fireball spent a turn");

	open.press(GameKey::NONE);
	exit_unless(
		open.mock.menus.empty(),
		"the inventory came back after the fireball was cancelled");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}
} // namespace

TEST(InventoryCloseAfterUseTest, ReadingTheTeleportScrollClosesTheInventory)
{
	const int childExitCode =
		exit_code_in_child(&reading_the_teleport_scroll_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, DrinkingAHealingPotionLeavesTheInventoryOpen)
{
	const int childExitCode = exit_code_in_child(
		&drinking_a_healing_potion_leaves_the_inventory_open);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(
	InventoryCloseAfterUseTest,
	UsingTheTeleportScrollFromTheRowMenuClosesTheInventory)
{
	const int childExitCode = exit_code_in_child(
		&using_the_teleport_scroll_from_the_row_menu_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, ClickingTheTeleportScrollClosesTheInventory)
{
	const int childExitCode =
		exit_code_in_child(&clicking_the_teleport_scroll_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, TheTeleportScrollLetterClosesTheInventory)
{
	const int childExitCode =
		exit_code_in_child(&the_teleport_scroll_letter_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

// The item's own answer, with no renderer, so it runs where the tests above skip.
TEST(InventoryCloseAfterUseTest, OnlyTheTeleportScrollSaysItMovesItsUser)
{
	EXPECT_TRUE(moves_its_user(ItemBehavior{ Teleporter{} }));
	EXPECT_FALSE(moves_its_user(ItemBehavior{ IdentifyScroll{} }));
}

TEST(InventoryCloseAfterUseTest, ReadingTheSleepScrollClosesTheInventory)
{
	const int childExitCode =
		exit_code_in_child(&reading_the_sleep_scroll_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, ReadingTheHoldPersonScrollClosesTheInventory)
{
	const int childExitCode = exit_code_in_child(
		&reading_the_hold_person_scroll_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, LightningWithATargetClosesTheInventory)
{
	const int childExitCode =
		exit_code_in_child(&lightning_with_a_target_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(
	InventoryCloseAfterUseTest,
	LightningWithNothingInRangeLeavesTheInventoryOpen)
{
	const int childExitCode = exit_code_in_child(
		&lightning_with_nothing_in_range_leaves_the_inventory_open);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(
	InventoryCloseAfterUseTest,
	AimingTheConfusionScrollClosesTheInventoryWhenTargetingEnds)
{
	const int childExitCode = exit_code_in_child(
		&aiming_the_confusion_scroll_closes_the_inventory_when_targeting_ends);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(InventoryCloseAfterUseTest, CancellingTheFireballCursorClosesTheInventory)
{
	const int childExitCode = exit_code_in_child(
		&cancelling_the_fireball_cursor_closes_the_inventory);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

// The items' own answers, with no renderer, so they run where the tests above skip. Every
// targeted scroll acts on creatures; the two aimed at a tile hand over to the cursor.
TEST(InventoryCloseAfterUseTest, EveryTargetedScrollSaysItTargetsCreatures)
{
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	for (
		const char* scrollKey : {
			"scroll_confusion",
			"scroll_fireball",
			"scroll_hold_person",
			"scroll_lightning",
			"scroll_sleep",
		})
	{
		const auto scroll =
			ItemCreator::create(scrollKey, Vector2D{ 0, 0 }, ctx);
		EXPECT_TRUE(targets_creatures(*scroll->behavior)) << scrollKey;
		EXPECT_FALSE(moves_its_user(*scroll->behavior)) << scrollKey;
	}
	EXPECT_FALSE(targets_creatures(ItemBehavior{ Teleporter{} }));
	EXPECT_FALSE(targets_creatures(ItemBehavior{ IdentifyScroll{} }));
}

TEST(InventoryCloseAfterUseTest, OnlyTheScrollsAimedAtATileHandOverToTargeting)
{
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	for (const char* scrollKey : { "scroll_confusion", "scroll_fireball" })
	{
		const auto scroll =
			ItemCreator::create(scrollKey, Vector2D{ 0, 0 }, ctx);
		EXPECT_TRUE(hands_over_to_targeting(*scroll->behavior)) << scrollKey;
	}
	for (
		const char* scrollKey : {
			"scroll_hold_person",
			"scroll_lightning",
			"scroll_sleep",
			"scroll_teleport",
		})
	{
		const auto scroll =
			ItemCreator::create(scrollKey, Vector2D{ 0, 0 }, ctx);
		EXPECT_FALSE(hands_over_to_targeting(*scroll->behavior)) << scrollKey;
	}
}

// end of file: InventoryCloseAfterUseTest.cpp
