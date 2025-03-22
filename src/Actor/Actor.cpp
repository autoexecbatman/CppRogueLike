// file: Actor.cpp
#include <iostream>
#include <string>
#include <algorithm>
#include <math.h>
#include <memory>
#include <gsl/gsl>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod.h>
#pragma warning (pop)

#include "../Game.h"
#include "../Ai/Ai.h"
#include "Actor.h"
#include "Attacker.h"
#include "Destructible.h"
#include "Pickable.h"
#include "Container.h"
#include "../Items.h"

//====
Actor::Actor(Vector2D position, ActorData data)
	:
	position(position),
	actorData(data)
{}

//====
void Actor::load(const json& j)
{
	position.x = j["position"]["x"];
	position.y = j["position"]["y"];
	direction.x = j["direction"]["x"];
	direction.y = j["direction"]["y"];
	actorData.ch = j["actorData"].at("ch").get<char>();
	actorData.name = j["actorData"].at("name").get<std::string>();
	actorData.color = j["actorData"].at("color").get<int>();

	// Deserialize vector of states
	for (const auto& state : j["states"])
	{
		states.push_back(state);
	}
}

void Actor::save(json& j)
{
	j["position"] = { {"y", position.y}, {"x", position.x} };
	j["direction"] = { {"y", direction.y}, {"x", direction.x} };
	j["actorData"] = {
		{"ch", actorData.ch},
		{"name", actorData.name},
		{"color", actorData.color}
	};

	// Serialize vector of states
	json statesJson;
	for (const auto& state : states)
	{
		statesJson.push_back(state);
	}
	j["states"] = statesJson;
}


// a function to get the Chebyshev distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	// using chebyshev distance
	const int distance = std::max(abs(position.x - tilePosition.x), abs(position.y - tilePosition.y));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

// the actor render function with color
void Actor::render() const noexcept
{
	if (is_visible())
	{
		attron(COLOR_PAIR(actorData.color));
		mvaddch(position.y, position.x, actorData.ch);
		attroff(COLOR_PAIR(actorData.color));
	}
}

// check if the actor is visible
bool Actor::is_visible() const noexcept
{
	return (!has_state(ActorState::FOV_ONLY) && game.map->is_explored(position)) || game.map->is_in_fov(position);
}

void Creature::load(const json& j)
{
	Actor::load(j); // Call base class load
	strength = j["strength"];
	dexterity = j["dexterity"];
	constitution = j["constitution"];
	intelligence = j["intelligence"];
	wisdom = j["wisdom"];
	charisma = j["charisma"];
	playerLevel = j["playerLevel"];
	gold = j["gold"];
	gender = j["gender"];
	weaponEquipped = j["weaponEquipped"];
	if (j.contains("attacker"))
	{
		attacker = std::make_unique<Attacker>("");
		attacker->load(j["attacker"]);
	}
	if (j.contains("destructible"))
	{
		destructible = Destructible::create(j["destructible"]);
	}
	if (j.contains("ai"))
	{
		ai = Ai::create(j["ai"]);
	}
	if (j.contains("container"))
	{
		container = std::make_unique<Container>(0);
		container->load(j["container"]);
	}
}

void Creature::save(json& j)
{
	Actor::save(j); // Call base class save
	j["strength"] = strength;
	j["dexterity"] = dexterity;
	j["constitution"] = constitution;
	j["intelligence"] = intelligence;
	j["wisdom"] = wisdom;
	j["charisma"] = charisma;
	j["playerLevel"] = playerLevel;
	j["gold"] = gold;
	j["gender"] = gender;
	j["weaponEquipped"] = weaponEquipped;
	if (attacker) {
		json attackerJson;
		attacker->save(attackerJson);
		j["attacker"] = attackerJson;
	}
	if (destructible) {
		json destructibleJson;
		destructible->save(destructibleJson);
		j["destructible"] = destructibleJson;
	}
	if (ai) {
		json aiJson;
		ai->save(aiJson);
		j["ai"] = aiJson;
	}
	if (container) {
		json containerJson;
		container->save(containerJson);
		j["container"] = containerJson;
	}
}

// the actor update
void Creature::update()
{
	// if the actor has an ai then update the ai
	if (ai)
	{
		ai->update(*this);
	}
}

void Creature::equip(Item& item)
{
	item.add_state(ActorState::IS_EQUIPPED);
	weaponEquipped = item.actorData.name;
}

void Creature::unequip(Item& item)
{
	item.remove_state(ActorState::IS_EQUIPPED);
	weaponEquipped = "None";
}

void Creature::pick()
{
	auto is_null = [](auto&& i) { return !i; };
	for (auto& i : game.container->inv)
	{
		if (i)
		{
			if (position == i->position)
			{
				if (container->add(std::move(i)))
				{
					std::erase_if(game.container->inv, is_null);
				}
			}
		}
	}
}

void Creature::drop()
{
	auto is_null = [](const auto& i) { return !i; };
	for (auto& i : container->inv)
	{
		if (i)
		{
			i->position = position;
			if (game.container->add(std::move(i)))
			{
				std::erase_if(container->inv, is_null);
			}
		}
	}
}

//==Item==
Item::Item(Vector2D position, ActorData data) : Object(position, data) {};

void Item::load(const json& j)
{
	Object::load(j); // Call base class load
	value = j["value"];
	if (j.contains("pickable")) {
		pickable = Pickable::create(j["pickable"]);
	}
}

void Item::save(json& j)
{
	Object::save(j); // Call base class save
	j["value"] = value;
	if (pickable) {
		json pickableJson;
		pickable->save(pickableJson);
		j["pickable"] = pickableJson;
	}
}

// end of file: Actor.cpp
