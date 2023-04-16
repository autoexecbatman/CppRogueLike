// file: Player.h
#ifndef PLAYER_H
#define PLAYER_H

#include <gsl/util>
#include "RandomDice.h"
#include "Map.h"
#include "Actor.h"

class Player : public Actor
{
public:
	Player(int y, int x);

	// Note::
	// X/Y coordinates set
	// in the function create_room()
	// in Map.cpp

	bool player_is_dead(); // TODO : correct this function
	void update();
	void draw();
};

#endif // !PLAYER_H
// end of file: Player.h