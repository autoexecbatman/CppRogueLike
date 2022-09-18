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

	//==ENGINE_FIELDS==
	// TODO : find a way to initialize properly.
	int screenHeight, screenWidth;
	int fovRadius;

	bool run = true;
	int lastKey = getch();

	std::deque<Actor*> actors;

	//==ENGINE_PROPERTIES==
	// TODO : initialize properly.
	Actor* player;
	Map* map;
	Gui* gui; // we make a pointer of the gui

	Engine(int screenWidth, int screenHeight);
	~Engine();

	void update();
	void render();
	
	void sendToBack(Actor* actor);
	Actor* getClosestMonster(int x, int y, double range) const;
	bool pickATile(int* x, int* y, float maxRange = 0.0f);

	void init();
	void load();
	void save();

	void print_container(const std::deque<Actor*> actors);
	
private:
	bool computeFov = false;
};

extern Engine engine;