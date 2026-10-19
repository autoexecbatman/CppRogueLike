#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "AbilityAllocation.h"
#include "InventoryData.h"
#include "PlayerRaceState.h"
#include "Stairs.h"

// Forward declarations
struct Game;
class Map;
class Minimap;
class CostOverlay;
struct Decoration;
class TileConfig;
class BodyPlanRegistry;
class SpellRegistry;
class MonsterRegistry;
class ItemRegistry;
class Gui;
class MessageSystem;
class RandomDice;
class CreatureManager;
class LevelManager;
class RenderingManager;
class InputHandler;
class GameStateManager;
class MenuManager;
class DisplayManager;
class GameLoopCoordinator;
class DataManager;
class TargetingSystem;
class HungerSystem;
class BuffSystem;
class FloatingTextSystem;
class AnimationSystem;
class ContentRegistry;
class DecorEditor;
class PrefabLibrary;
#ifndef __EMSCRIPTEN__
class ContentEditor;
class RoomEditor;
class ItemEditor;
class MonsterEditor;
class SpellEditor;
#endif
class Stairs;
class TileFeature;
class Trap;
class SpellTile;
class BaseMenu;
#include "Vector2D.h"
struct DungeonRoom;
class Renderer;
class InputSystem;
class Creature;
class Player;

namespace Pathfinding
{
class Pathfinder;
}

// Where the turn is. Moving it on is routine, so nothing final is kept here.
enum class GameStatus
{
	STARTUP,
	IDLE,
	NEW_TURN
};

// How the run ended. Kept apart from GameStatus so that a turn moving on cannot undo a
// death or a win; only the start of a new run sets it back to UNDECIDED.
enum class RunOutcome
{
	UNDECIDED,
	VICTORY,
	DEFEAT
};

enum class WindowState
{
	MENU,
	GAME,
#ifndef __EMSCRIPTEN__
	ROOM_EDITOR,
	MONSTER_EDITOR,
	SPELL_EDITOR,
	ITEM_EDITOR,
#endif
};

class GameState
{
private:
	bool run{ true };
	bool shouldSave{ true };
	bool isLoadedGame{ false };
	// The clock, in time units. A round is TIME_UNITS_PER_ROUND of them, so the round
	// number is read off this rather than counted beside it.
	int time{ 0 };

	// How many rounds have had their upkeep run. Not derivable from the clock: the
	// clock says what time it is and this says what has already happened.
	int roundsRun{ 0 };

	GameStatus gameStatus{ GameStatus::STARTUP };
	RunOutcome runOutcome{ RunOutcome::UNDECIDED };
	WindowState windowState{ WindowState::GAME };

public:
	bool get_run() const noexcept { return run; }

	void set_run(bool newValue) noexcept { run = newValue; }

	bool get_should_save() const noexcept { return shouldSave; }

	void set_should_save(bool newValue) noexcept { shouldSave = newValue; }

	bool get_is_loaded_game() const noexcept { return isLoadedGame; }

	void set_is_loaded_game(bool newValue) noexcept { isLoadedGame = newValue; }

	// The clock in time units. Advanced only by an action being spent, which is
	// spend_player_action and nothing else.
	int get_time() const noexcept { return time; }

	void set_time(int newValue) noexcept { time = newValue; }

	void advance_clock(int timeUnits) noexcept { time += timeUnits; }

	// Rounds whose upkeep has run. apply_round_upkeep raises it to the round the
	// clock has reached, so an action spanning several rounds feeds all of them.
	int get_rounds_run() const noexcept { return roundsRun; }

	void set_rounds_run(int newValue) noexcept { roundsRun = newValue; }

	GameStatus get_game_status() const noexcept { return gameStatus; }

	void set_game_status(GameStatus newStatus) noexcept { gameStatus = newStatus; }

	// Written by the end of a run - Player::die for DEFEAT, the Amulet of Yendor for
	// VICTORY - and set back to UNDECIDED only by begin_run.
	//
	// Example:
	//   gameState.set_run_outcome(RunOutcome::DEFEAT);
	//   gameState.set_game_status(GameStatus::NEW_TURN); // the turn moves on
	//   gameState.get_run_outcome();                     // -> RunOutcome::DEFEAT still
	RunOutcome get_run_outcome() const noexcept { return runOutcome; }

	void set_run_outcome(RunOutcome outcome) noexcept { runOutcome = outcome; }

	// Starts a run, new or loaded: the turn at STARTUP and no outcome yet. The one place
	// an outcome is forgotten.
	//
	// Example:
	//   gameState.set_run_outcome(RunOutcome::DEFEAT);
	//   gameState.begin_run();
	//   gameState.get_run_outcome(); // -> RunOutcome::UNDECIDED
	//   gameState.get_game_status(); // -> GameStatus::STARTUP
	void begin_run() noexcept
	{
		gameStatus = GameStatus::STARTUP;
		runOutcome = RunOutcome::UNDECIDED;
	}

	WindowState get_window_state() const noexcept { return windowState; }

	void set_window_state(WindowState newState) noexcept { windowState = newState; }
};

// Data collected by character creation menus before Player is constructed.
// Menus write here; GameStateManager::init_new_game consumes it.
struct PlayerBlueprint
{
	std::string gender{ "None" };
	std::string name{ "Player" };
	PlayerRaceState playerRaceState{ PlayerRaceState::NONE };

	// The six as Method VI allocated them, before the race is paid. They start
	// where the method starts them, so a blueprint that never reached the
	// allocation screen still builds a legal character rather than a blank one.
	// The class the menu picked, and the only record of it: its name is derived by
	// class_display_name. MONSTER until a class is chosen, which the Player
	// constructor refuses.
	CreatureClass creatureClass{ CreatureClass::MONSTER };

	std::array<int, ABILITY_COUNT> abilityScores{
		METHOD_SIX_STARTING_SCORE,
		METHOD_SIX_STARTING_SCORE,
		METHOD_SIX_STARTING_SCORE,
		METHOD_SIX_STARTING_SCORE,
		METHOD_SIX_STARTING_SCORE,
		METHOD_SIX_STARTING_SCORE
	};
};

/**
 * GameContext - Dependency injection container
 *
 * Replaces global `extern Game game;` with explicit dependency passing.
 * Phase 1: Expand structure with all Game systems
 * Phase 2: Add get_context() to Game class
 * Phase 3: Replace game->X with ctx->X incrementally (1,196 references)
 */
struct GameContext
{
	// Core game world
	Map* map{ nullptr };
	Gui* gui{ nullptr };
	// The player, stored once as the handle that owns it. Set by Game::context();
	// init_new_game and load_all assign through it to replace the player.
	std::unique_ptr<Player>* playerOwner{ nullptr };

	// The player as a creature, which is what almost every caller wants. Computed
	// from playerOwner rather than stored, so there is no second copy to fall out
	// of step with it. Null before the player exists.
	[[nodiscard]] Creature* player() const;

	// The player as itself, for the few callers that need a class name, a kill
	// count or an equipment slot. Asserts a player exists.
	[[nodiscard]] Player& player_concrete() const;

	// Character creation data - populated by menus, consumed by init_new_game.
	PlayerBlueprint* playerBlueprint{ nullptr };

	// Core systems
	MessageSystem* messageSystem{ nullptr };
	RandomDice* dice{ nullptr };

	// Managers
	CreatureManager* creatureManager{ nullptr };
	LevelManager* levelManager{ nullptr };
	RenderingManager* renderingManager{ nullptr };
	InputHandler* inputHandler{ nullptr };
	GameStateManager* stateManager{ nullptr };
	MenuManager* menuManager{ nullptr };
	DisplayManager* displayManager{ nullptr };
	GameLoopCoordinator* gameLoopCoordinator{ nullptr };
	DataManager* dataManager{ nullptr };

	// Rendering
	Renderer* renderer{ nullptr };
	InputSystem* inputSystem{ nullptr };

	// Specialized systems
	TargetingSystem* targeting{ nullptr };
	HungerSystem* hungerSystem{ nullptr };
	BuffSystem* buffSystem{ nullptr };
	FloatingTextSystem* floatingText{ nullptr };
	AnimationSystem* animSystem{ nullptr };
	ContentRegistry* contentRegistry{ nullptr };
	Minimap* minimap{ nullptr };
	CostOverlay* costOverlay{ nullptr };
	Pathfinding::Pathfinder* pathfinder{ nullptr };
	DecorEditor* decorEditor{ nullptr };
	PrefabLibrary* prefabLibrary{ nullptr };
#ifndef __EMSCRIPTEN__
	ContentEditor* contentEditor{ nullptr };
	RoomEditor* roomEditor{ nullptr };
	ItemEditor* itemEditor{ nullptr };
	MonsterEditor* monsterEditor{ nullptr };
	SpellEditor* spellEditor{ nullptr };
#endif
	const TileConfig* tileConfig{ nullptr };
	const BodyPlanRegistry* bodyPlanRegistry{ nullptr };
	SpellRegistry* spellRegistry{ nullptr };
	MonsterRegistry* monsterRegistry{ nullptr };
	ItemRegistry* itemRegistry{ nullptr };

	// Game world data
	Stairs* stairs{ nullptr };
	// Traps and spell tiles are separate systems that happen to share a floor:
	// a trap hides, triggers and is disarmed; a spell tile answers to the spell
	// that made it. Nothing needs to ask which one it is holding.
	std::vector<std::unique_ptr<Trap>>* traps{ nullptr };
	std::vector<std::unique_ptr<SpellTile>>* spellTiles{ nullptr };
	std::vector<std::unique_ptr<Decoration>>* decorations{ nullptr };
	FloorInventory* floorInventory{ nullptr };
	// Every creature on the level except the player, who is reached through player().
	std::vector<std::unique_ptr<Creature>>* creatures{ nullptr };
	std::vector<DungeonRoom>* rooms{ nullptr };

	// UI Collections
	std::deque<std::unique_ptr<BaseMenu>>* menus{ nullptr };

	// Mouse path overlay - written by AiPlayer, read by RenderingManager
	std::vector<Vector2D>* mousePathOverlay{ nullptr };

	// Game state
	GameState* gameState{ nullptr };
};
