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

	Actor* player;
	Actor* stairs;
	Map* map;
	Gui* gui;

	bool run = true;
	int lastKey = getch();
	int keyPress = getch();

	std::deque<Actor*> actors;

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
	
};

extern Engine engine;

#endif // PROJECT_PATH_ENGINE_H_