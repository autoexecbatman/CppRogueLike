// file: RunEndTest.cpp
// A run that ends takes the screen in the tick it ends, whatever was open. A death or a
// win reached inside a menu used to leave every menu open, so a dead player went on using
// the pack until the last menu closed, and the end screen then waited for a key.
//
// Each test runs Game::tick in a child process with tests/mocks/HiddenRenderer.h, because
// the tick opens a frame, and skips with the reason where there is no OpenGL driver.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RunEndTest.*

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <typeinfo>

#include "src/Ai.h"
#include "src/ArmorClass.h"
#include "src/BaseMenu.h"
#include "src/Colors.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/DeathMenu.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/NotificationMenu.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"

namespace
{
// A menu that stays open and does nothing, standing for the inventory beneath a scroll.
class OpenMenu : public BaseMenu
{
public:
	void menu([[maybe_unused]] GameContext& ctx) override {}
};

// A menu whose one tick kills the player, as a scroll of fireball read at its reader's own
// tile does from inside the targeting cursor.
class FatalMenu : public BaseMenu
{
public:
	void menu(GameContext& ctx) override
	{
		ctx.player()->set_hp(0);
		ctx.player()->die(ctx);
	}
};

// A menu whose one tick wins the run, as the Amulet of Yendor does from the inventory.
class WinningMenu : public BaseMenu
{
public:
	void menu(GameContext& ctx) override
	{
		ctx.gameState->set_run_outcome(RunOutcome::VICTORY);
	}
};

// A monster whose turn kills the player outright, standing for any blow that lands.
class KillingAi : public Ai
{
public:
	void update([[maybe_unused]] Creature& owner, GameContext& ctx) override
	{
		ctx.player()->set_hp(0);
		ctx.player()->die(ctx);
	}

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::MONSTER; }

	void load([[maybe_unused]] const json& j) override {}

	void save([[maybe_unused]] json& j) override {}
};

// Loads what main does before a game can start, then starts one with a human fighter and
// plays its first turn, so the next key is an ordinary turn of an ordinary game.
void start_a_game(Game& game, GameContext& ctx, int& loopNum)
{
	game.tileConfig.load(Paths::TILE_CONFIG);
	game.bodyPlanRegistry.load(Paths::BODY_PLANS);
	game.spellRegistry.load(Paths::SPELLS);
	game.monsterRegistry.load(Paths::MONSTERS);
	game.itemRegistry.load(Paths::ITEMS);
	game.itemRegistry.load_enhanced_rules(Paths::ENHANCED_RULES);
	game.init_world();
	game.palette.load(Paths::TILE_CONFIG);
	game.prefabLibrary.load(Paths::PREFABS);

	game.playerBlueprint.creatureClass = CreatureClass::FIGHTER;
	game.playerBlueprint.playerRaceState = PlayerRaceState::HUMAN;
	game.stateManager.init_new_game(ctx);

	ctx.inputSystem->queue_key(GameKey::WAIT, 0);
	game.tick(loopNum);
}

// A player with what the end screens read: a health pool, an armour class, experience,
// and a class the death screen can name.
void give_the_game_a_player(Game& game)
{
	game.player = std::make_unique<Player>(Vector2D{ 1, 1 });
	game.player->healthPool = std::make_unique<HealthPool>(10);
	game.player->armorClass = std::make_unique<ArmorClass>(10);
	game.player->experienceReward = std::make_unique<ExperienceReward>(0);
	game.player->set_creature_class(CreatureClass::FIGHTER);
}

// Loads the game's own renderer the way the hidden one is loaded, and the tile names a
// screen draws by, for a test that lets an end screen draw on the next tick.
void load_the_games_renderer(Game& game)
{
	game.tileConfig.load(Paths::TILE_CONFIG);
	game.renderer.load_font(Paths::resolve(Paths::DAWNLIKE_FONT).string(), HIDDEN_RENDERER_FONT_SIZE);
	game.renderer.load_dawnlike(Paths::resolve(Paths::DAWNLIKE_DIR).string());
	game.renderer.update_viewport();
}

// The player dies inside a menu with another open beneath it: after the one tick, the
// death screen is all that is on the stack.
[[noreturn]] void a_death_under_a_menu_takes_the_screen()
{
	HiddenRenderer hidden{};
	Game game;
	give_the_game_a_player(game);
	game.menus.push_back(std::make_unique<OpenMenu>());
	game.menus.push_back(std::make_unique<FatalMenu>());

	int loopNum{ 0 };
	game.tick(loopNum);

	exit_unless(game.menus.size() == 1, "the menus open when the player died are still open");
	exit_unless(typeid(*game.menus.back()) == typeid(DeathMenu), "the death screen is not what is left on the stack");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// The run is won inside a menu with another open beneath it: after the one tick, the
// victory screen is all that is on the stack.
[[noreturn]] void a_win_under_a_menu_takes_the_screen()
{
	HiddenRenderer hidden{};
	Game game;
	give_the_game_a_player(game);
	game.menus.push_back(std::make_unique<OpenMenu>());
	game.menus.push_back(std::make_unique<WinningMenu>());

	int loopNum{ 0 };
	game.tick(loopNum);

	exit_unless(game.menus.size() == 1, "the menus open when the run was won are still open");
	exit_unless(typeid(*game.menus.back()) == typeid(NotificationMenu), "the victory screen is not what is left on the stack");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// The death screen itself ticks with the run already lost; the run ended on an earlier
// tick, so this one leaves the stack alone rather than clearing it again.
[[noreturn]] void the_death_screen_is_not_taken_down_by_its_own_outcome()
{
	HiddenRenderer hidden{};
	Game game;
	load_the_games_renderer(game);
	give_the_game_a_player(game);
	game.menus.push_back(std::make_unique<OpenMenu>());
	game.menus.push_back(std::make_unique<FatalMenu>());

	int loopNum{ 0 };
	game.tick(loopNum);
	exit_unless(game.menus.size() == 1, "the first tick did not leave the death screen alone on the stack");
	const BaseMenu* deathScreen = game.menus.back().get();

	game.tick(loopNum);

	exit_unless(!game.menus.empty() && game.menus.back().get() == deathScreen, "the death screen was replaced by its own outcome");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// No menu is open; the player waits and a monster's turn in that round kills them. The
// death screen is on the stack after that one tick, without another key.
[[noreturn]] void a_death_in_a_monsters_round_takes_the_screen()
{
	HiddenRenderer hidden{};
	Game game;
	GameContext ctx = game.context();
	int loopNum{ 0 };
	start_a_game(game, ctx, loopNum);
	exit_unless(game.menus.empty(), "the new game's first turn left a menu open, so this is not a turn with no menu");
	exit_unless(!game.player->is_dead(), "the player died before the monster was placed");

	auto killer = std::make_unique<Creature>(game.player->position + Vector2D{ 1, 0 }, ActorData{ TileRef{}, "killer", ColorPairId::WHITE_BLACK });
	killer->healthPool = std::make_unique<HealthPool>(10);
	killer->armorClass = std::make_unique<ArmorClass>(10);
	killer->ai = std::make_unique<KillingAi>();
	game.creatures.push_back(std::move(killer));

	ctx.inputSystem->queue_key(GameKey::WAIT, 0);
	game.tick(loopNum);

	exit_unless(game.player->is_dead(), "the monster's turn did not come in the round the player waited through");
	exit_unless(game.menus.size() == 1 && typeid(*game.menus.back()) == typeid(DeathMenu), "the death screen is not on the stack after the fatal round");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Runs a child body and turns its exit code into a skip or a result.
void expect_child_passes(void (*childBody)())
{
	// A lost run deletes the save, as permadeath does, so a test that loses one would
	// delete a real save sitting where the game keeps it. It skips rather than do that.
	if (std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE)))
	{
		GTEST_SKIP() << "a save exists at " << Paths::resolve(Paths::SAVE_FILE).string()
					 << "; a lost run would delete it, so this test refuses to run";
	}

	const int childExitCode = exit_code_in_child(childBody);
	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}
} // namespace

TEST(RunEndTest, ADeathUnderAMenuTakesTheScreen)
{
	expect_child_passes(&a_death_under_a_menu_takes_the_screen);
}

TEST(RunEndTest, AWinUnderAMenuTakesTheScreen)
{
	expect_child_passes(&a_win_under_a_menu_takes_the_screen);
}

TEST(RunEndTest, ADeathInAMonstersRoundTakesTheScreen)
{
	expect_child_passes(&a_death_in_a_monsters_round_takes_the_screen);
}

TEST(RunEndTest, TheDeathScreenIsNotTakenDownByItsOwnOutcome)
{
	expect_child_passes(&the_death_screen_is_not_taken_down_by_its_own_outcome);
}

// end of file: RunEndTest.cpp
