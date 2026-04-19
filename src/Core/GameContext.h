#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../Actor/InventoryData.h"
#include "../Actor/Stairs.h"

// Forward declarations
struct Game;
class Map;
class Minimap;
struct Decoration;
class TileConfig;
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
#ifndef EMSCRIPTEN
class ContentEditor;
class RoomEditor;
class ItemEditor;
class MonsterEditor;
class SpellEditor;
#endif
class Stairs;
class Object;
class BaseMenu;
#include "../Utils/Vector2D.h"
struct DungeonRoom;
class Renderer;
class InputSystem;
class Creature;
class Player;
class Dijkstra;

enum class GameStatus
{
	STARTUP,
	IDLE,
	NEW_TURN,
	VICTORY,
	DEFEAT
};

enum class WindowState
{
	MENU,
	GAME,
#ifndef EMSCRIPTEN
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
	int time{ 0 };
	GameStatus gameStatus{ GameStatus::STARTUP };
	WindowState windowState{ WindowState::GAME };

public:
	bool get_run() const noexcept { return run; }
	void set_run(bool v) noexcept { run = v; }

	bool get_should_save() const noexcept { return shouldSave; }
	void set_should_save(bool v) noexcept { shouldSave = v; }

	bool get_is_loaded_game() const noexcept { return isLoadedGame; }
	void set_is_loaded_game(bool v) noexcept { isLoadedGame = v; }

	int get_time() const noexcept { return time; }
	void set_time(int v) noexcept { time = v; }
	void increment_time() noexcept { ++time; }

	GameStatus get_game_status() const noexcept { return gameStatus; }
	void set_game_status(GameStatus s) noexcept { gameStatus = s; }

	WindowState get_window_state() const noexcept { return windowState; }
	void set_window_state(WindowState s) noexcept { windowState = s; }

};

// Data collected by character creation menus before Player is constructed.
// Menus write here; GameStateManager::init_new_game consumes it.
struct PlayerBlueprint
{
	std::string gender{ "None" };
	std::string name{ "Player" };
	std::string playerClass{ "None" };
	std::string playerRace{ "None" };
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
	Creature* player{ nullptr };

	// Ownership handle — set by Game::context(), used by init_new_game/load_all
	// to construct and assign the player without going through Game directly.
	std::unique_ptr<Player>* playerOwner{ nullptr };

	// Character creation data — populated by menus, consumed by init_new_game.
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
	Dijkstra* pathfinder{ nullptr };  // Persistent pathfinding object (reused across turns)
	DecorEditor* decorEditor{ nullptr };
	PrefabLibrary* prefabLibrary{ nullptr };
#ifndef EMSCRIPTEN
	ContentEditor* contentEditor{ nullptr };
	RoomEditor* roomEditor{ nullptr };
	ItemEditor* itemEditor{ nullptr };
	MonsterEditor* monsterEditor{ nullptr };
	SpellEditor* spellEditor{ nullptr };
#endif
	const TileConfig* tileConfig{ nullptr };

	// Game world data
	Stairs* stairs{ nullptr };
	std::vector<std::unique_ptr<Object>>* objects{ nullptr };
	std::vector<std::unique_ptr<Decoration>>* decorations{ nullptr };
	struct InventoryData* inventoryData{ nullptr };
	std::vector<std::unique_ptr<class Creature>>* creatures{ nullptr };
	std::vector<DungeonRoom>* rooms{ nullptr };

	// UI Collections
	std::deque<std::unique_ptr<BaseMenu>>* menus{ nullptr };

	// Mouse path overlay — written by AiPlayer, read by RenderingManager
	std::vector<Vector2D>* mousePathOverlay{ nullptr };

	// Game state
	GameState* gameState{ nullptr };
};
