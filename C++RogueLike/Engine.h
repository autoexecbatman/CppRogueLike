#pragma once

#include <deque>
#include <curses.h>
#include "Gui.h"

//====
//the enumeration for the controls of the player
enum CONTROLS
{
	UP = 'w', DOWN = 's', LEFT = 'a', RIGHT = 'd', QUIT = 'q'
};

class Engine
{
public:
	enum GameStatus
	{
		STARTUP,
		IDLE,
		NEW_TURN,
		VICTORY,
		DEFEAT
	} gameStatus;
	int lastKey = getch();
	std::deque<Actor*> actors;
	Actor* player;
	Map* map;
	int fovRadius = 0;
	int screenWidth = 0;
	int screenHeight = 0;
	Gui* gui; // we make a pointer of the gui

	Engine(int screenWidth, int screenHeight);
	~Engine();
	void update();
	void render();
	void sendToBack(Actor* actor);
	void print_container(const std::deque<Actor*> actors);
	Actor* getClosestMonster(int x, int y, double range) const;
	bool pickATile(int* x, int* y, float maxRange = 0.0f);
private:
	bool computeFov = false;
};

extern Engine engine;