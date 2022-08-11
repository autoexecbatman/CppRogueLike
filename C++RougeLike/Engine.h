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
	std::vector<Actor*> actors;
	Actor* player;
	Map* map;
	int fovRadius;


	Engine();
	~Engine();
	void update();
	void render();
private:
	bool computeFov;
};
extern Engine engine;

//Engine engine;