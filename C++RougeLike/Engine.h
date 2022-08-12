#pragma once

#include <vector>

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

	std::vector<Actor*> actors;
	Actor* player;
	Map* map;
	int fovRadius = 0;


	Engine();
	~Engine();
	void update();
	void render();
private:
	bool computeFov = false;
};

extern Engine engine;