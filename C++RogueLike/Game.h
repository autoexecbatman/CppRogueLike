// file: Game.h
#ifndef GAME_H
#define GAME_H

#pragma once

#include <memory>
#include <span>

#include "Gui/Gui.h"
#include "Map/Map.h"
#include "Colors/Colors.h"
#include "Actor/Actor.h"
#include "ActorTypes/Player.h"
#include "Ai/AiShopkeeper.h"
#include "ChatGPT.h"
#include "Weapons.h"

class Game
{
private:
	std::vector<std::pair<int, std::string>> attackMessageParts; // this vector holds the parts of the attack message

public:
	std::vector <std::vector<std::pair<int, std::string>>> attackMessagesWhole; // this vector holds all of the attack messages
	std::string messageToDisplay{ "Init Message" };
	int messageColor{ EMPTY_PAIR };

	bool run{ true };
	bool shouldSave{ true };
	enum class GameStatus
	{
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus{ GameStatus::STARTUP };

	std::unique_ptr<Stairs> stairs;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{0, 0}, 0, 0, "A", 0, 0, 0, 0, 0, 0) };
	Creature* shopkeeper{ nullptr };

	std::unique_ptr<ChatGPT> chatGPT{ std::make_unique<ChatGPT>() };

	std::unique_ptr<Map> map{ std::make_unique<Map>(MAP_HEIGHT, MAP_WIDTH) };
	const std::unique_ptr<Gui> gui{ std::make_unique<Gui>() };


	int keyPress{ 0 };
	int lastKey{ 0 };

	int dungeonLevel{ 0 };

	std::vector<std::unique_ptr<Creature>> actors; // a vector of actors
	std::vector< std::unique_ptr<Object>> objects; // a vector of objects
	//std::vector< std::unique_ptr<Item>> items; // a vector of items
	std::unique_ptr<Container> container{ std::make_unique<Container>(0) };

	std::vector<Weapons> weapons; // a vector of weapons
	std::vector<StrengthAttributes> strengthAttributes; // a vector of strength attributes

	// Public member functions.
	void init();
	void create_player();
	void update();
	void render();

	void update_creatures(std::span<std::unique_ptr<Creature>> creatures)
	{
		for (const auto& creature : creatures)
		{
			if (creature && creature->is_visible())
			{
				creature->update();
			}
		}
	}
	void render_creatures(std::span<std::unique_ptr<Creature>> creatures)
	{
		for (const auto& actor : actors)
		{
			if (actor && actor->is_visible())
			{
				actor->render();
				std::clog << "Actor: " << actor->actorData.name << " is drawn" << std::endl;
			}
		}
	}
	void render_items(std::span<std::unique_ptr<Item>> creatures)
	{
		for (const auto& item : container->inv)
		{
			if (item && item->is_visible())
			{
				item->render();
				std::clog << "Item: " << item->actorData.name << " is drawn" << std::endl;
			}
		}
	}

	/*void send_to_back(Actor& actor);*/

	template<typename T>
	void send_to_back(T& actor)
	{
		auto actorIsInVector = [&actor](const auto& a) noexcept { return a.get() == &actor; }; // lambda to check if the actor is in the vector
		auto it = std::find_if(actors.begin(), actors.end(), actorIsInVector); // get the iterator of the actor
		const auto distance = std::distance(actors.begin(), it); // get the distance from the begining of the vector to the actor
		for (auto i = distance; i > 0; i--)
		{
			// swap actor with the previous actor
			std::swap(gsl::at(actors, i - 1), gsl::at(actors, i));
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

	void key_store() { std::clog << "storing key" << std::endl; lastKey = keyPress; }
	void key_listen() { std::clog << "getting key" << std::endl; keyPress = getch(); }

	// the player goes down stairs
	void next_level();
	Creature* get_actor(Vector2D pos) const noexcept;
	void dispay_levelup(int level);
	void display_character_sheet() noexcept;

	//==DEBUG FUNCTIONS==//
	void wizard_eye() noexcept; // prints Actors names instead of their ASCII chars
	void err(std::string_view e) noexcept { if (debugMode) { clear(); mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2, e.data()); refresh(); getch(); } }
	void enableDebugMode() noexcept { debugMode = true; }
	void disableDebugMode() noexcept { debugMode = false; }
	void log(std::string_view message);
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
