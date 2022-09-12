#pragma once
//#include "Container.h"

//====
// a class for the moving characters
class Actor
{
public:
	int y = 0, x = 0; // position on map
	int ch = -47; // the symbol to print
	int col = 0; // color for the actor
	const char* name = "actor_name"; // add name
	bool blocks = false; // does the actor blocks movement?
	
	Attacker* attacker; // the actor can attack
	Destructible* destructible; // the actor can be destroyed
	Ai* ai; // the actor can have AI
	Container* container; // the actor can be a container
	Pickable* pickable;
	
	Actor(
		int y,
		int x,
		int ch,
		const char* name,
		int col
	);

	~Actor();

	void update(); // update() will handle the monster turn.
	double getDistance(int cx, int cy) const; // a function to get the distance from an actor to a specific tile of the map
	void render() const; // render the actor on the screen.
	void pickItem(int x, int y); // pick up an item
};

//====