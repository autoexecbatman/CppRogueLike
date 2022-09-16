#include <iostream>
#include <vector>

#include "main.h"

Container::Container(int inv_size) : inv_size(inv_size)
{
	/*std::clog << "Container::Container(int size)" << std::endl;*/
}

Container::~Container() 
{
	inventory.clear();
}

// checks that the container is not full.
bool Container::add(Actor* actor)
{
	std::clog << "Container::add();" << std::endl;
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

void Container::load(TCODZip& zip)
{
	inv_size = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0) {
		Actor* actor = new Actor(0, 0, 0, NULL, 0);
		actor->load(zip);
		inventory.push_back(actor);
		nbActors--;
	}
}

void Container::save(TCODZip& zip)
{
	zip.putInt(inv_size);
	zip.putInt(inventory.size());
	// iterate through the inventory and save the item
	for (Actor* actor : inventory)
	{
		actor->save(zip);
	}
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