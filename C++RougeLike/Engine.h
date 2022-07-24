#pragma once

#include <vector>

enum CONTROLS
{
	UP = 'w', DOWN = 's', LEFT = 'a', RIGHT = 'd', QUIT = 'q'
};

class Engine
{
public:
	std::vector<Actor*> actors;
	Actor* player;
	Map* map;

	Engine();
	~Engine();
	void update();
	void render();
};
//extern Engine engine;

//Engine engine;