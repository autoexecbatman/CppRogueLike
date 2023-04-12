#ifndef PROJECT_PATH_PLAYER_H_
#define PROJECT_PATH_PLAYER_H_

//class Actor;
#include "Actor.h"

class Player : public Actor
{
	Player(int y, int x);

	bool player_is_dead();
	void update();
	void draw();
};

#endif // !PROJECT_PATH_PLAYER_H_