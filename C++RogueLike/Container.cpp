#include <iostream>
#include <vector>

#include "main.h"

Container::Container(int invSize) : invSize(invSize) {}

Container::~Container() 
{
	inventoryList.clear();
}

// checks that the container is not full.
bool Container::add(Actor* actor)
{	
	//if (invSize < 0 && inventory.size() <= invSize)
	//{
	//	// inventory full
	//	return false;
	//}

	if (// check if the inventory is full
		invSize > 0
		&& 
		inventoryList.size() >= static_cast<unsigned int>(invSize)
		)
	{
		// inventory full return false
		return false;
	}

	inventoryList.push_back(actor); // add the actor to the inventory

	return true;
}


void Container::remove(Actor* actor)
{
	inventoryList.erase(std::remove(inventoryList.begin(), inventoryList.end(), actor), inventoryList.end());
}

void Container::load(TCODZip& zip)
{
	invSize = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0) 
	{
		Actor* actor = new Actor(0, 0, 0, nullptr, 0);
		actor->load(zip);
		inventoryList.push_back(actor);
		nbActors--;
	}
}

void Container::save(TCODZip& zip)
{
	zip.putInt(invSize);
	zip.putInt(inventoryList.size());
	// iterate through the inventory and save the item
	for (Actor* actor : inventoryList)
	{
		actor->save(zip);
	}
}

void Container::print_container(std::vector<Actor*> container)
{
	int i = 0;
	for (const auto& item : inventoryList)
	{
		std::cout << item->name << i << " ";
		i++;
	}
	std::cout << '\n';
}