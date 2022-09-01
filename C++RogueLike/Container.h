#pragma once
//#include <vector>
//#include "main.h"

class Container
{
public:
	int inv_size;
	/*TCODList<Actor*> inventory;*/
	std::vector<Actor*> inventory;

	Container(int inv_size);
	~Container();
	// checks that the container is not full.
	bool add(Actor* actor);
	void remove(Actor* actor);

	void print_container(std::vector<Actor*> container);
};