#ifndef PLAYER_H
#define PLAYER_H

// should it take coordinates as parameters? Yes. Because the player is should move using the keyboard.
// should the player be a pointer? No. Because pointers are used to point to objects, not to be objects themselves.
// the player is a unique object, it should not be copied. True. Unless you want multiple user controlled objects like player.
// should the player be a class? Yes. Because the player is an actor.
// should it be added to a container of Actors? No. The plan is to have only one player.
// should it share ai with other actors?
// what to extract from the player pointer in the game class?
// I want the player not to be a pointer, but I want to be able to access it from the game class.
// I want to be able to access the player from the game class, but I want to be able to access the game class from the player class.


//class Actor;
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