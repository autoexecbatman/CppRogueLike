#pragma once

#include <deque>
#include <memory>
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
#ifndef EMSCRIPTEN
class ContentEditor;
class DecorEditor;
class RoomEditor;
class ItemEditor;
class MonsterEditor;
class SpellEditor;
class PrefabLibrary;
#endif
class Stairs;
class Object;
class BaseMenu;
struct Vector2D;
struct DungeonRoom;
class Renderer;
class InputSystem;
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

/**
 * GameContext - Dependency injection container
 *
 * Replaces global `extern Game game;` with explicit dependency passing.
 * Phase 1: Expand structure with all Game systems
 * Phase 2: Add get_context() to Game class
 * Phase 3: Replace game.X with ctx->X incrementally (1,196 references)
 */
struct GameContext
{
	// Core game world
	Map* map{ nullptr };
	Gui* gui{ nullptr };
	Player* player{ nullptr };

	// Core systems
	MessageSystem* messageSystem{ nullptr };
	RandomDice* dice{ nullptr };

	// Managers
	CreatureManager* creature_manager{ nullptr };
	LevelManager* level_manager{ nullptr };
	RenderingManager* rendering_manager{ nullptr };
	InputHandler* input_handler{ nullptr };
	GameStateManager* state_manager{ nullptr };
	MenuManager* menu_manager{ nullptr };
	DisplayManager* display_manager{ nullptr };
	GameLoopCoordinator* game_loop_coordinator{ nullptr };
	DataManager* data_manager{ nullptr };

	// Rendering
	Renderer* renderer{ nullptr };
	InputSystem* input_system{ nullptr };

	// Specialized systems
	TargetingSystem* targeting{ nullptr };
	HungerSystem* hunger_system{ nullptr };
	BuffSystem* buff_system{ nullptr };
	FloatingTextSystem* floating_text{ nullptr };
	AnimationSystem* anim_system{ nullptr };
	ContentRegistry* content_registry{ nullptr };
	Minimap* minimap{ nullptr };
	Dijkstra* pathfinder{ nullptr };  // Persistent pathfinding object (reused across turns)
#ifndef EMSCRIPTEN
	ContentEditor* content_editor{ nullptr };
	DecorEditor* decor_editor{ nullptr };
	RoomEditor* room_editor{ nullptr };
	ItemEditor* item_editor{ nullptr };
	MonsterEditor* monster_editor{ nullptr };
	SpellEditor* spell_editor{ nullptr };
	PrefabLibrary* prefab_library{ nullptr };
#endif
	const TileConfig* tile_config{ nullptr };

	// Game world data
	Stairs* stairs{ nullptr };
	std::vector<std::unique_ptr<Object>>* objects{ nullptr };
	std::vector<std::unique_ptr<Decoration>>* decorations{ nullptr };
	struct InventoryData* inventory_data{ nullptr };
	std::vector<std::unique_ptr<class Creature>>* creatures{ nullptr };
	std::vector<DungeonRoom>* rooms{ nullptr };

	// UI Collections
	std::deque<std::unique_ptr<BaseMenu>>* menus{ nullptr };

	// Game state
	GameState* game_state{ nullptr };
};
