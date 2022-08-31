#include <iostream>
#include <vector>

#include "main.h"

Container::Container(int inv_size) : inv_size(inv_size)
{
	std::clog << "Container::Container(int size)" << std::endl;
}

Container::~Container() 
{
	inventory.clear();
}

// checks that the container is not full.
bool Container::add(Actor* actor)
{
	if (inv_size < 0 && inventory.size() <= unsigned(inv_size))
	{
		// inventory full
		return false;
	}
	inventory.push_back(actor);
	// print inventory list in curses
	print_container(inventory);
	return true;
}


void Container::remove(Actor* actor)
{
	inventory.erase(std::remove(inventory.begin(), inventory.end(), actor), inventory.end());
}

void Container::print_container(std::vector<Actor*> container)
{
	int i = 0;
	for (const auto& item : inventory)
	{
		std::cout << item->name << i << " ";
		i++;
	}
	std::cout << '\n';
}