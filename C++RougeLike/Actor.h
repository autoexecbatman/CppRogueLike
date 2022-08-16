#pragma once

//====
// a class for the moving characters
class Actor
{
public:
	int y = 0, x = 0; // position on map
	int ch = -47; // the symbol to print
	int col = 0; // color for the actor
	const char* name = "init_name"; // add name
	bool blocks = false; // does the actor blocks movement?
	
	Attacker* attacker; // the actor can attack
	Destructible* destructible; // the actor can be destroyed
	Ai* ai; // the actor can have AI
	
	Actor(
		int y,
		int x,
		int ch,
		const char* name,
		int col
	);

	void update(); // update() will handle the monster turn.
	void render() const; // render the actor on the screen.
};

//====