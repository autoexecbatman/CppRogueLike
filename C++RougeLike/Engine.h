#pragma once

#include <vector>
#include <deque>
#include <curses.h>

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


	Engine(int screenWidth, int screenHeight);
	~Engine();
	void update();
	void render();
	void sendToBack(Actor* actor);
private:
	bool computeFov = false;
};

extern Engine engine;