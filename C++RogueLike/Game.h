// file: Game.h
#ifndef GAME_H
#define GAME_H

#pragma once

#include <memory> // std::shared_ptr, std::make_shared

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
	enum class GameStatus : int
	{
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus{ GameStatus::STARTUP };

	std::unique_ptr<Player> player_unique{ std::make_unique<Player>(0, 0, 0, 0, "", 0, 0, 0, 0, 0, 0, false) };
	Player* player{ player_unique.get() };
	Actor* shopkeeper{ nullptr };

	std::unique_ptr<Actor> stairs_unique{ std::make_unique<Actor>(
		0, // int posX
		0, // int posY
		'>', // char symbol
		"stairs", // std::string name
		WHITE_PAIR, // int colorPair
		1 // int index
	) };
	Actor* stairs{ stairs_unique.get() };

	std::unique_ptr<ChatGPT> chatGPT{ std::make_unique<ChatGPT>() };

	std::unique_ptr<Map> map{ std::make_unique<Map>(MAP_HEIGHT, MAP_WIDTH) };
	const std::unique_ptr<Gui> gui{ std::make_unique<Gui>() };


	int keyPress{ 0 }; // stores the current key pressed
	int lastKey{ 0 }; // stores that was pressed before the current key

	int dungeonLevel{ 0 };

	std::vector<std::unique_ptr<Actor>> actors; // a vector of actors

	std::vector<Weapons> weapons; // a vector of weapons

	// Public member functions.
	void init();
	void create_player();
	void update();
	void render();
	void send_to_back(Actor& actor);
	Actor* get_closest_monster(int fromPosX, int fromPosY, double inRange) const noexcept;
	bool pick_tile(int* x, int* y, int maxRange);
	void run_menus();

	bool mouse_moved() noexcept;
	void target();
	void load_all(); // does not override Persistent::load()
	void save_all(); // does not override Persistent::save()

	void key_store() { std::clog << "storing key" << std::endl; lastKey = keyPress; }
	void key_listen() { std::clog << "getting key" << std::endl; keyPress = getch(); }

	// the player goes down stairs
	void next_level();
	const std::unique_ptr<Actor>& get_actor(int x, int y) const noexcept;
	void dispay_levelup(int level);
	void display_character_sheet() noexcept;

	//==DEBUG FUNCTIONS==//
	void wizard_eye() noexcept; // prints Actors names instead of their ASCII chars
	void err(const std::string& e) noexcept { clear(); mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2, e.c_str()); refresh(); getch(); }
	void enableDebugMode() noexcept { debugMode = true; }
	void disableDebugMode() noexcept { debugMode = false; }
	void log(const std::string& message);

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
void print_container(const std::vector<std::shared_ptr<T>>& container)
{
	int i = 0;
	for (const auto& item : container)
	{
		std::clog << i << ". " << item->name << " ";
		i++;
	}
	std::clog << '\n';
}

#endif // !GAME_H
// end of file: Game.h
