#ifndef PROJECT_PATH_ENGINE_H_
#define PROJECT_PATH_ENGINE_H_

#include <deque>

#include "Map.h"
#include "Actor.h"
#include "Gui.h"
#include "Literals.h"





//==ENGINE==
// The engine class is the main class of the game.
// It will handle the game loop and the game states.
// ( Length screenWidth , Length screenHeight ) : the width and height of the screen.
class Engine
{
public:

	//==GAME_STATUS==
	// enumerates the current game status.
	enum class GameStatus : int
	{
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus;

	int fovRadius;

	//==ENGINE_FIELDS==

	Length screenWidth;
	Length screenHeight;

	//==ENGINE_PROPERTIES==

	std::unique_ptr<Actor> player;
	std::unique_ptr<Actor> stairs;
	Map* map;
	Gui* gui;

	bool run = true;
	int lastKey = getch();
	int keyPress = getch();

	/*std::deque<Actor*> actors;*/

	std::unordered_map<int, std::unique_ptr<Actor>> actors;
	std::deque<int> actorsOrder;

	int nextId = 0;

	template <class T>
	auto new_actor(
		int y,
		int x,
		char ch,
		const char* name,
		int col
	) -> int
	{
		actors.emplace(nextId, std::make_unique<T>) (y, x, ch, name, col);
		actorsOrder.push_back(nextId);

		return nextId++;
	}


	//template <class T>
	//auto new_actor(
	//	int y,
	//	int x,
	//	char ch,
	//	const char* name,
	//	int col,
	//	int maxHp,
	//	int defense,
	//	int power,
	//	int xp
	//) -> int
	//{
	//	actors.try_emplace(nextId, std::make_unique<T>) (y, x, ch, name, col, maxHp, defense, power, xp);
	//	actorsOrder.push_back(nextId);

	//	return nextId++;
	//}

	//==ENGINE_METHODS==
	
	void update();
	void render();
	
	void send_to_back(Actor* actor);

	Actor* get_closest_monster(int fromPosX, int fromPosY, double inRange) const;
	
	bool pick_tile(int* x, int* y, float maxRange = 0.0f);

	void game_menu();
	bool mouse_moved();
	void target();
	void load();
	void save();
	
	void term();

	void print_container(const std::deque<Actor*> actors);
	
	void key_listener() { keyPress = getch(); }
	
	Engine(Length screenWidth, Length screenHeight);
	~Engine();
	
	void init();
private:

	bool computeFov = false;

public:
	
	int level;
	void next_level();

	Actor* get_actor(int x, int y) const;
	
	void dispay_stats(int level);
	void display_character_sheet();

	int random_number(int min, int max);
	void wizard_eye();
	
	void delete_actor(int id);

};

extern Engine engine;

#endif // PROJECT_PATH_ENGINE_H_