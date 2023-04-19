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
	void setPosX(int x) noexcept { posX = x; }
	void setPosY(int y) noexcept { posY = y; }
	int getPosX() const noexcept { return posX; }
	int getPosY() const noexcept { return posY; }
	void player_get_pos_from_map();
};

#endif // !PLAYER_H
// end of file: Player.h