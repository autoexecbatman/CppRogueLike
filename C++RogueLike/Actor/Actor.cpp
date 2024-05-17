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

//====
Actor::Actor(Vector2D position, ActorData data, ActorFlags flags)
	:
	position(position),
	actorData(data),
	flags(flags),
	attacker(nullptr),
	destructible(std::make_unique<Destructible>(0, 0, "", 0, 0, 0)),
	ai(nullptr),
	container(nullptr),
	pickable(nullptr)
{}

Actor::~Actor() = default;

//====
void Actor::load(TCODZip& zip)
{
	// this block assigns the values from the zip file to the actor
	position.x = zip.getInt();
	position.y = zip.getInt();

	actorData.ch = zip.getInt();
	actorData.color = zip.getInt();
	actorData.name = _strdup(zip.getString());

	flags.blocks = zip.getInt();

	// this block assigns checks if the actor has a component
	const bool hasAttacker{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasDestructible{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasAi{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasContainer{ gsl::narrow_cast<bool>(zip.getInt()) };

	// this block assigns the values from the zip file to the actor's components if they exist. 
	// Is this a double assignment? No, because the components are not assigned in the constructor.
	if (hasAttacker) { attacker = std::make_unique<Attacker>(0,0,0); attacker->load(zip); }
	/*if (hasDestructible) { destructible = Destructible::create(zip); }*/
	if (hasAi) { ai = Ai::create(zip); }
	if (hasPickable) { pickable = Pickable::create(zip); }
	if (hasContainer) { container = std::make_unique<Container>(0); container->load(zip); }
}

void Actor::save(TCODZip& zip)
{
	// Add a debug log to display the actor name
	game.log("Saving actor: " + actorData.name);

	zip.putInt(position.x);
	zip.putInt(position.y);
	zip.putInt(actorData.ch);
	zip.putInt(actorData.color);
	zip.putString(actorData.name.c_str());
	zip.putInt(flags.blocks);

	zip.putInt(attacker != nullptr);
	/*zip.putInt(destructible != nullptr);*/
	zip.putInt(ai != nullptr);
	zip.putInt(pickable != nullptr);
	zip.putInt(container != nullptr);

	if (attacker) attacker->save(zip);
	/*if (destructible) destructible->save(zip);*/
	if (ai) ai->save(zip);
	if (pickable) pickable->save(zip);
	if (container) container->save(zip);
}

// the actor render function with color
void Actor::render() const noexcept
{
	attron(COLOR_PAIR(actorData.color));
	mvaddch(position.y, position.x, actorData.ch);
	attroff(COLOR_PAIR(actorData.color));
}

// adds an item to the actor's inventory
void Actor::pickItem(int x, int y)
{
	// add item to inventory
	/*container->add(std::move(*this));*/
}

void Actor::equip(Actor& item)
{
	item.flags.isEquipped = true;
	weaponEquipped = item.actorData.name;
}

void Actor::unequip(Actor& item)
{
	item.flags.isEquipped = false;
	weaponEquipped = "None";
}

bool Actor::is_visible() const noexcept
{
	return (!flags.fovOnly && game.map->is_explored(position.x, position.y)) || game.map->is_in_fov(position);
}

// the actor update
void Actor::update()
{
	// if the actor has an ai then update the ai
	if (ai)
	{
		ai->update(*this);
	}
}

// a function to get the distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	// using chebyshev distance
	/*const int distance = std::max(abs(posX - tileX), abs(posY - tileY));*/
	const int distance = std::max(abs(position.x - tilePosition.x), abs(position.y - tilePosition.y));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

// end of file: Actor.cpp
