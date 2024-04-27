// file: Container.cpp
#include <vector>
#include <gsl/util>

#include "../Game.h"
#include "Container.h"
#include "Actor.h"

Container::Container(size_t invSize) noexcept : invSize(invSize) {}

Container::~Container() noexcept
{
	inventoryList.clear();
}

// checks that the container is not full.
bool Container::add(std::unique_ptr<Actor> actor)
{	
	// Do not add the player to the inventory
	if (actor.get() == game.player) { return false; }

	// check if the inventory is full then return false
	if (invSize > 0 && inventoryList.size() >= invSize) { return false; }

	// if inventory is not full try add the actor to the inventory
	inventoryList.push_back(std::move(actor));

	return true;
}

// remove an item from the inventory
void Container::remove(std::unique_ptr<Actor> actor)
{
	game.log("Removing item from inventory");
	game.actors.push_back(std::move(actor));
	game.send_to_back(*game.actors.back().get());
	inventoryList.erase(std::remove_if(inventoryList.begin(), inventoryList.end(), [](const auto& a) noexcept { return !a; }), inventoryList.end());
	game.log("Item removed from inventory");
}

void Container::load(TCODZip& zip)
{
	invSize = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0)
	{
		auto actor = std::make_unique<Actor>(0, 0, 0, "", 0, 0);
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
	for (const auto& actor : inventoryList)
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
