// file: Container.cpp
#include <iostream>
#include <vector>
#include <gsl/util>

#include "Container.h"
#include "Actor.h"
#include "Game.h"

Container::Container(int invSize) noexcept : invSize(invSize) {}

Container::~Container() noexcept
{
	inventoryList.clear();
}

// checks that the container is not full.
bool Container::add(Actor& actor)
{	

	// Do not add the player to the inventory
	if (&actor == game.player.get())
	{
		return false;
	}

	// check if the inventory is full
	if (invSize > 0 && inventoryList.size() >= gsl::narrow_cast<unsigned int>(invSize))
	{
		// inventory full return false
		return false;
	}

	// if inventory is not full try add the actor to the inventory and catch any exceptions
	try
	{
		inventoryList.push_back(std::make_shared<Actor>(actor));
	}
	catch (const std::bad_alloc& e)
	{
		game.log("Failed to add " + actor.name + " to your inventory. System out of memory.");
	}
	catch (const std::exception& e)
	{
		game.log("Failed to add " + actor.name + " to your inventory. Error: " + e.what());
	}

	return true;
}

// remove an item from the inventory
void Container::remove(Actor& actor)
{
	std::clog << "Removing item from inventory" << std::endl;

	auto it = std::find_if(
		inventoryList.begin(),
		inventoryList.end(),
		[&](const std::shared_ptr<Actor>& item) 
		{return item.get() == &actor;}
	);

	if (it == inventoryList.end()) {
		std::cout << "Item not found in inventory" << std::endl;
		std::clog << "Item not found in inventory" << std::endl;
		return;
	}

	inventoryList.erase(it);
	std::clog << "Item removed from inventory" << std::endl;
}

void Container::load(TCODZip& zip)
{
	invSize = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0)
	{
		std::shared_ptr<Actor> actor = std::make_shared<Actor>(0, 0, 0, "", 0, 0);
		actor->load(zip);
		inventoryList.push_back(std::move(actor));
		nbActors--;
	}
}

void Container::save(TCODZip& zip)
{
	zip.putInt(invSize);
	const int nbActors = gsl::narrow_cast<int>(inventoryList.size());
	zip.putInt(nbActors);
	// iterate through the inventory and save the item
	for (std::shared_ptr<Actor> actor : inventoryList)
	{
		actor->save(zip); // Unhandled exception at 0x632F9A85 (libtcod.dll) in C++RogueLike.exe: 0xC00000FD: Stack overflow (parameters: 0x00000001, 0x00442FFC).
	}
}

void Container::print_container(std::vector<std::shared_ptr<Actor>> container)
{
	int i = 0;
	for (const auto& item : inventoryList)
	{
		std::cout << item->name.c_str() << i << " ";
		i++;
	}
	std::cout << '\n';
}

// end of file: Container.cpp
