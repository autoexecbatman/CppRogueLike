// file: Game.h
#ifndef GAME_H
#define GAME_H

#include <memory> // std::shared_ptr, std::make_shared

#include "Actor.h"
#include "Gui.h"
#include "Map.h"
#include "Colors.h"
#include "ChatGPT.h"

class Game
{
public:
	bool run{ true };
	enum class GameStatus : int
	{
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus{ GameStatus::STARTUP };

	std::shared_ptr<Actor> player{ std::make_shared<Actor>(
		25, // int posX
		40, // int posY
		'@', // char symbol
		"Player", // std::string name
		PLAYER_PAIR, // int colorPair
		0 // int index
	) };

	std::shared_ptr<Actor> stairs{ std::make_shared<Actor>(
		0, // int posX
		0, // int posY
		'>', // char symbol
		"stairs", // std::string name
		WHITE_PAIR, // int colorPair
		1 // int index
	) };

	std::unique_ptr<ChatGPT> chatGPT{ std::make_unique<ChatGPT>() };

	std::unique_ptr<Map> map{ std::make_unique<Map>(MAP_HEIGHT, MAP_WIDTH) };
	const std::unique_ptr<Gui> gui{ std::make_unique<Gui>() };


	int keyPress{ 0 }; // stores the current key pressed
	int lastKey{ 0 }; // stores that was pressed before the current key

	int dungeonLevel{ 0 };

	std::vector<std::shared_ptr<Actor>> actors; // a vector of actors

	// Public member functions.
	void init();
	void update();
	void render();
	void send_to_back(Actor& actor);
	std::shared_ptr<Actor> get_closest_monster(int fromPosX, int fromPosY, double inRange) const;
	bool pick_tile(int* x, int* y, int maxRange);

	bool mouse_moved();
	void target();
	void load_all(); // does not override Persistent::load()
	void save_all(); // does not override Persistent::save()

	void key_store() { std::clog << "storing key" << std::endl; lastKey = keyPress; }
	void key_listen() { std::clog << "getting key" << std::endl; keyPress = getch(); }

	void next_level();
	std::shared_ptr<Actor> get_actor(int x, int y) const;
	void dispay_levelup(int level);
	void display_character_sheet() noexcept;
	int random_number(int min, int max);
	void wizard_eye() noexcept;

private:
	// Private member variables.
	bool computeFov = false;

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
