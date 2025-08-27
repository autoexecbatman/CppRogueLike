// file: Game.h
#ifndef GAME_H
#define GAME_H

#pragma once

#include <iostream>
#include <memory>
#include <span>
#include <deque>
#include <format>

#include "Gui/Gui.h"
#include "Gui/LogMessage.h"
#include "Map/Map.h"
#include "Colors/Colors.h"
#include "Actor/Actor.h"
#include "ActorTypes/Player.h"
#include "Ai/AiShopkeeper.h"
#include "Items/Weapons.h"
#include "Menu/BaseMenu.h"
#include "Systems/TargetingSystem.h"
#include "Systems/HungerSystem.h"
#include "Systems/LevelUpSystem.h"
#include "Systems/MessageSystem.h"
#include "Systems/RenderingManager.h"
#include "Systems/InputHandler.h"
#include "Systems/GameStateManager.h"
#include "Systems/LevelManager.h"
#include "Attributes/StrengthAttributes.h"
#include "Attributes/DexterityAttributes.h"
#include "Attributes/ConstitutionAttributes.h"

class Game
{
public:

	bool run{ true };
	bool shouldSave{ true };
	bool gameWasInit{ false };
	int time{ 0 };
	bool shouldTakeInput{ true };
	enum class GameStatus
	{
		STARTUP, IDLE, NEW_TURN, VICTORY, DEFEAT
	}
	gameStatus{ GameStatus::STARTUP };
	enum class WindowState
	{
		MENU, GAME
	}
	windowState{ WindowState::GAME };
	RandomDice d; // Random number generator.
	TargetingSystem targeting;
	HungerSystem hunger_system;
	MessageSystem message_system;
	RenderingManager rendering_manager;
	InputHandler input_handler;
	GameStateManager state_manager;
	LevelManager level_manager;

	Map map{ Map{MAP_HEIGHT, MAP_WIDTH} };
	Gui gui{};

	std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D {0, 0}) };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{0, 0}) };

	std::vector<Vector2D> rooms; // room coordinates after bsp
	std::vector<std::unique_ptr<Creature>> creatures; // a vector of actors
	std::vector< std::unique_ptr<Object>> objects; // a vector of objects
	std::unique_ptr<Container> container{ std::make_unique<Container>(0) };

	// for loading from json
	std::vector<Weapons> weapons{ load_weapons() }; // a vector of weapons
	std::vector<StrengthAttributes> strengthAttributes{ load_strength_attributes() }; // a vector of strength attributes
	std::vector<DexterityAttributes> dexterityAttributes{ load_dexterity_attributes() }; // a vector of dexterity attributes
	std::vector<ConstitutionAttributes> constitutionAttributes{ load_constitution_attributes() }; // a vector of constitution attributes

	// Menu container for que
	std::deque<std::unique_ptr<BaseMenu>> menus;
	std::deque<std::unique_ptr<BaseMenu>> deadMenus;
	
	// Public member functions.
	void init();
	void update();
	// Rendering delegated to RenderingManager
	void render() { rendering_manager.render_world(map, *stairs, objects, *container, creatures, *player); }
	void update_creatures(std::span<std::unique_ptr<Creature>> creatures);
	void cleanup_dead_creatures(); // Remove dead creatures from the game
	void spawn_creatures() const;
	void handle_menus();
	void handle_gameloop(Gui& gui, int loopNum);
	void handle_ranged_attack();

	void display_help() noexcept;

	// EMSCRIPTEN COMPATIBILITY FUNCTIONS
	void safe_screen_clear() { rendering_manager.safe_screen_clear(); }     // Safe clear for web environment
	void force_screen_refresh() { rendering_manager.force_screen_refresh(); }  // Force refresh for Emscripten
	void restore_game_display() { render(); gui.gui_render(); rendering_manager.force_screen_refresh(); }  // Restore clean game state

	Web* findWebAt(Vector2D position);

	void add_debug_weapons_at_player_feet();

	template <typename T>
	void create_creature(Vector2D position)
	{
		creatures.push_back(std::make_unique<T>(position));
	}

	template <typename T>
	void create_item(Vector2D position)
	{
		container->inv.push_back(std::make_unique<T>(position));
	}

	/*void send_to_back(Actor& actor);*/
	template<typename T>
	void send_to_back(T& actor)
	{
		auto actorIsInVector = [&actor](const auto& a) noexcept { return a.get() == &actor; }; // lambda to check if the actor is in the vector
		auto it = std::find_if(creatures.begin(), creatures.end(), actorIsInVector); // get the iterator of the actor
		const auto distance = std::distance(creatures.begin(), it); // get the distance from the begining of the vector to the actor
		for (auto i = distance; i > 0; i--)
		{
			// swap actor with the previous actor
			std::swap(creatures[i - 1], creatures[i]);
		}
	}

	Creature* get_closest_monster(Vector2D fromPosition, double inRange) const noexcept;
	bool pick_tile(Vector2D* position, int maxRange);

	// Game state management delegated to GameStateManager
	void load_all();
	void save_all();
	void next_level() { level_manager.advance_to_next_level(map, *player, message_system); gameStatus = GameStatus::STARTUP; }

	Creature* get_actor(Vector2D pos) const noexcept;
	void display_levelup(int level);
	void display_character_sheet() const noexcept;

	//==DEBUG FUNCTIONS==//
	void wizard_eye() noexcept; // prints Actors names instead of their ASCII chars
	void err(std::string_view e) noexcept { if (message_system.is_debug_mode()) { clear(); mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2, e.data()); refresh(); getch(); } }

	//==MESSAGE FUNCTIONS==//
	// Delegated to MessageSystem
	void message(int color, std::string_view text, bool isComplete = false) { message_system.message(color, text, isComplete); }
	void append_message_part(int color, std::string_view text) { message_system.append_message_part(color, text); }
	void finalize_message() { message_system.finalize_message(); }
	void transfer_messages_to_gui() { message_system.transfer_messages_to_gui(gui); }
	void log(std::string_view message) const { message_system.log(message); }
	void display_debug_messages() noexcept { message_system.display_debug_messages(); }
	void enable_debug_mode() noexcept { message_system.enable_debug_mode(); }
	void disable_debug_mode() noexcept { message_system.disable_debug_mode(); }

private:
	// Private member variables.
	bool computeFov{ false };
	// Private member functions.
};

// Declaration of the global engine object.
extern Game game;

template<typename T>
void print_container(std::span<std::unique_ptr<T>> container)
{
	int i = 0;
	for (const auto& item : container)
	{
		std::cout << i << ". " << item->name << " ";
		i++;
	}
	std::cout << '\n';
}

#endif // !GAME_H
// end of file: Game.h
