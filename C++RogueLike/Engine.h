#pragma once

#include <deque>

#include "Gui.h"

//==ENGINE==
// the engine class
class Engine
{
public:
	Engine(int screenWidth, int screenHeight);
	~Engine();

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
	Map* map;
	Gui* gui;

	bool run = true;
	int lastKey = getch();
	int key = getch();

	std::deque<Actor*> actors;


	void update();
	void render();
	
	void sendToBack(Actor* actor);
	Actor* getClosestMonster(int x, int y, double range) const;
	bool pickATile(int* x, int* y, float maxRange = 0.0f);

	void init();
	void load();
	void save();
	
	void term();

	void print_container(const std::deque<Actor*> actors);

	void game_menu();

	void key_listener() { key = getch(); }
	
private:
	bool computeFov = false;
};

extern Engine engine;