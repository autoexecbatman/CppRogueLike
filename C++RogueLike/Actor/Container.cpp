// file: Container.cpp
#include <vector>
#include <gsl/util>

#include "../Game.h"
#include "Container.h"
#include "Actor.h"

Container::Container(size_t invSize) noexcept : invSize(invSize) {}

// checks that the container is not full.
bool Container::add(std::unique_ptr<Item> actor)
{	
	// check if the inventory is full then return false
	if (invSize > 0 && inv.size() >= invSize) { return false; }

	// if inventory is not full try add the actor to the inventory
	inv.push_back(std::move(actor));

	return true;
}

// remove an item from the inventory
void Container::remove(std::unique_ptr<Item> actor)
{
	game.log("Removing item from inventory");
	game.container->add(std::move(actor));
	game.send_to_back(*game.actors.back().get());
	inv.erase(std::remove_if(inv.begin(), inv.end(), [](const auto& a) noexcept { return !a; }), inv.end());
	game.log("Item removed from inventory");
}

void Container::load(TCODZip& zip)
{
	invSize = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0)
	{
		auto actor = std::make_unique<Item>(Vector2D(0, 0), ActorData(), ActorFlags());
		actor->load(zip);
		inv.push_back(std::move(actor));
		nbActors--;
	}
}

void Container::save(TCODZip& zip)
{
	zip.putInt(invSize);
	const size_t nbActors = inv.size();
	zip.putInt(nbActors);
	// iterate through the inventory and save the item
	for (const auto& actor : inv)
	{
		actor->save(zip); // Unhandled exception at 0x632F9A85 (libtcod.dll) in C++RogueLike.exe: 0xC00000FD: Stack overflow (parameters: 0x00000001, 0x00442FFC).
	}
}

void Container::print_container(std::span<std::unique_ptr<Actor>> container)
{
	int i = 0;
	for (const auto& item : inv)
	{
		std::cout << item->actorData.name << i << " ";
		i++;
	}
	std::cout << '\n';
}

// end of file: Container.cpp
