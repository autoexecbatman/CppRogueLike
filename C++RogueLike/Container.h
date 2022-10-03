#pragma once

class Container
{
public:
	int invSize = 0;

	std::vector<Actor*> inventoryList;

	Container(int invSize);
	~Container();
	// checks that the container is not full.
	bool add(Actor* actor);
	void remove(Actor* actor);

	void load(TCODZip& zip);
	void save(TCODZip& zip);

	void print_container(std::vector<Actor*> container);
};