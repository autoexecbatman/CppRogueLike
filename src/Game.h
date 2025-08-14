// file: Game.h
#ifndef GAME_H
#define GAME_H

#pragma once

#include <iostream>
#include <memory>
#include <span>
#include <deque>

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
#include "Attributes/StrengthAttributes.h"
#include "Attributes/DexterityAttributes.h"
#include "Attributes/ConstitutionAttributes.h"

class Game
{
private:
	std::vector<LogMessage> attackMessageParts; // this vector holds the parts of the attack message
public:
	std::vector <std::vector<LogMessage>> attackMessagesWhole; // this vector holds all of the attack messages
	std::string messageToDisplay{ "Init Message" };
	int messageColor{ WHITE_BLACK_PAIR };

	bool run{ true };
	bool shouldSave{ true };
	bool gameInit{ false };
	int time{ 0 };
	bool shouldInput{ true };
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

	std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D {0, 0}) };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{0, 0}) };

	std::unique_ptr<Map> map{ std::make_unique<Map>(MAP_HEIGHT, MAP_WIDTH) };
	const std::unique_ptr<Gui> gui{ std::make_unique<Gui>() };

	std::vector<Vector2D> rooms; // room coordinates after bsp
	std::vector<std::unique_ptr<Creature>> creatures; // a vector of actors
	std::vector< std::unique_ptr<Object>> objects; // a vector of objects
	std::unique_ptr<Container> container{ std::make_unique<Container>(0) };

	// for loading from json
	std::vector<Weapons> weapons{ loadWeapons() }; // a vector of weapons
	std::vector<StrengthAttributes> strengthAttributes{ loadStrengthAttributes() }; // a vector of strength attributes
	std::vector<DexterityAttributes> dexterityAttributes{ loadDexterityAttributes() }; // a vector of dexterity attributes
	std::vector<ConstitutionAttributes> constitutionAttributes{ loadConstitutionAttributes() }; // a vector of constitution attributes

	// Menu container for que
	std::deque<std::unique_ptr<BaseMenu>> menus;
	std::deque<std::unique_ptr<BaseMenu>> deadMenus;
	
	// Public member functions.
	void init();
	void update();
	void render();
	void update_creatures(std::span<std::unique_ptr<Creature>> creatures);
	void cleanup_dead_creatures(); // Remove dead creatures from the game
	void render_creatures(std::span<std::unique_ptr<Creature>> creatures);
	void spawn_creatures() const;
	void render_items(std::span<std::unique_ptr<Item>> items);
	void handle_menus();
	void handle_gameloop(Gui& gui, int loopNum);
	void handle_ranged_attack();

	void display_help() noexcept;

	// EMSCRIPTEN COMPATIBILITY FUNCTIONS
	void safe_screen_clear();     // Safe clear for web environment
	void force_screen_refresh();  // Force refresh for Emscripten
	void restore_game_display();  // Restore clean game state

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
	void run_menus();

	bool mouse_moved() noexcept;
	Vector2D get_mouse_position() noexcept;
	Vector2D get_mouse_position_old() noexcept;
	void target();
	void load_all();
	void save_all();

	int keyPress{ 0 };
	int lastKey{ 0 };
	// C++ Core Guidelines F.6: noexcept for functions that cannot throw
	void key_store() noexcept { std::clog << "storing key" << std::endl; lastKey = keyPress; }
	void key_listen() noexcept { std::clog << "getting key" << std::endl; keyPress = getch(); }

	// the player goes down stairs
	int dungeonLevel{ 1 };
	int shopkeepersOnCurrentLevel{ 0 }; // Track shopkeepers per level
	void next_level();
	Creature* get_actor(Vector2D pos) const noexcept;
	void dispay_levelup(int level);
	void display_character_sheet() noexcept;

	//==DEBUG FUNCTIONS==//
	void wizard_eye() noexcept; // prints Actors names instead of their ASCII chars
	void err(std::string_view e) noexcept { if (debugMode) { clear(); mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2, e.data()); refresh(); getch(); } }
	void enableDebugMode() noexcept { debugMode = true; }
	void disableDebugMode() noexcept { debugMode = false; }
	void log(std::string_view message) const;
	void display_debug_messages() noexcept;

	//==MESSAGE FUNCTIONS==//
	void message(int color, const std::string& text, bool isComplete);
	void appendMessagePart(int color, const std::string& text);
	void finalizeMessage();
	void transferMessagesToGui();


private:
	// Private member variables.
	bool computeFov{ false };
	bool debugMode{ true };
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
