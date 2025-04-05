// file: Container.cpp
#include <vector>
#include <gsl/util>

#include "../Game.h"
#include "Container.h"
#include "Actor.h"
#include "../Items.h"

Container::Container(size_t invSize) noexcept : invSize(invSize) {}

// checks that the container is not full.
bool Container::add(std::unique_ptr<Item> actor)
{
	// check if the inventory is full then return false
	if (invSize > 0 && inv.size() >= invSize) {
		// Log message and notify the player that inventory is full
		game.log("Inventory full! Cannot add more items.");
		game.message(WHITE_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return false;
	}

	// if inventory is not full try add the actor to the inventory
	inv.push_back(std::move(actor));

	return true;
}

// remove an item from the inventory
void Container::remove(std::unique_ptr<Item> actor)
{
	game.log("Removing item from inventory");
	game.container->add(std::move(actor));
	game.send_to_back(*game.creatures.back().get());
	auto is_null = [](const auto& a) noexcept { return !a; };
	std::erase_if(inv, is_null);
	game.log("Item removed from inventory");
}

void Container::load(const json& j)
{
	// Deserialize the inventory size
	invSize = j["invSize"];

	// Deserialize the inventory
	for (const auto& actorJson : j["inv"])
	{
		auto actor = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{});
		actor->load(actorJson);
		inv.push_back(std::move(actor));
	}
}

void Container::save(json& j)
{
	// Serialize the inventory size
	j["invSize"] = invSize;

	// Serialize the inventory
	j["inv"] = json::array();
	for (const auto& actor : inv)
	{
		json actorJson;
		actor->save(actorJson);
		j["inv"].push_back(actorJson);
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