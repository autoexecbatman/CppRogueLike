#pragma once

#include <deque>

#include "Gui.h"

//==ENGINE==
// the engine class
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

	int screenWidth;
	int screenHeight;

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

	void init();
	void game_menu();
	bool mouse_moved();
	void target();
	void load();
	void save();
	
	void term();

	void print_container(const std::deque<Actor*> actors);


	void key_listener() { keyPress = getch(); }
	
	Engine(int screenWidth, int screenHeight);
	~Engine();
	
private:

	bool computeFov = false;

public:
	
	int level;
	void next_level();
	Actor* get_actor(int x, int y) const;
	void dispay_stats(int level);
	void display_character_sheet();

	int random_number(int min, int max);
	
};

extern Engine engine;