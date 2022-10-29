#ifndef PROJECT_PATH_ACTOR_H_
#define PROJECT_PATH_ACTOR_H_

#include "libtcod.hpp"
#include "Attacker.h"
#include "Destructible.h"
#include "Ai.h"
#include "Container.h"
#include "Pickable.h"


//==Actor==
// a class for the actors in the game
// (player, monsters, items, etc.)
class Actor : public Persistent
{
public:
	int posY = 0, posX = 0; // position on map
	char ch = -47; // the symbol to print
	int col = 0; // color for the actor
	const char* name = "actor_name"; // add name
	bool blocks = false; // does the actor blocks movement?
	bool fovOnly = false; // to make some actors visible when not in fov
	
	std::unique_ptr<Attacker> attacker; // the actor can attack
	Destructible* destructible; // the actor can be destroyed
	Ai* ai; // the actor can have AI
	Container* container; // the actor can be a container
	Pickable* pickable; // the actor can be picked
	
	Actor(
		int y,
		int x,
		int ch,
		const char* name,
		int col
	);

	~Actor();

	void load(TCODZip& zip);
	void save(TCODZip& zip);

	void update(); // update() will handle the monster turn.

	int get_distance(int tileX, int tileY) const; // a function to get the distance from an actor to a specific tile of the map

	void render() const; // render the actor on the screen.
	void pickItem(int x, int y); // pick up an item
};

#endif // !PROJECT_PATH_ACTOR_H_
//====