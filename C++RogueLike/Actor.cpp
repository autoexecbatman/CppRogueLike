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

#include "Actor.h"
#include "Attacker.h"
#include "Destructible.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Game.h"

//====
Actor::Actor(
	int y,
	int x,
	int ch,
	std::string name,
	int col,
	int index
) : 
	posY(y),
	posX(x),
	ch(ch),
	col(col),
	index(index),
	name(name),
	blocks(true),
	fovOnly(true),
	canSwim(false),
	attacker(nullptr),
	destructible(nullptr),
	ai(nullptr),
	container(nullptr),
	pickable(nullptr)
{}

Actor::~Actor() {}

//====
void Actor::load(TCODZip& zip)
{
	// this block assigns the values from the zip file to the actor
	posX = zip.getInt();
	posY = zip.getInt();
	ch = zip.getInt();
	col = zip.getInt();
	name = _strdup(zip.getString());
	blocks = zip.getInt();

	// this block assigns checks if the actor has a component
	const bool hasAttacker{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasDestructible{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasAi{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasContainer{ gsl::narrow_cast<bool>(zip.getInt()) };

	// this block assigns the values from the zip file to the actor's components if they exist. 
	// Is this a double assignment? No, because the components are not assigned in the constructor.
	if (hasAttacker) { attacker = std::make_unique<Attacker>(0,0,0); attacker->load(zip); }
	if (hasDestructible) { destructible = Destructible::create(zip); }
	if (hasAi) { ai = Ai::create(zip); }
	if (hasPickable) { pickable = Pickable::create(zip); }
	if (hasContainer) { container = std::make_unique<Container>(0); container->load(zip); }
}

void Actor::save(TCODZip& zip)
{
	// Add a debug log to display the actor name
	game.log("Saving actor: " + name);

	zip.putInt(posX);
	zip.putInt(posY);
	zip.putInt(ch);
	zip.putInt(col);
	zip.putString(name.c_str());
	zip.putInt(blocks);

	zip.putInt(attacker != nullptr);
	zip.putInt(destructible != nullptr);
	zip.putInt(ai != nullptr);
	zip.putInt(pickable != nullptr);
	zip.putInt(container != nullptr);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
	if (pickable) pickable->save(zip);
	if (container) container->save(zip);
}

// the actor render function with color
void Actor::render() const noexcept
{
	attron(COLOR_PAIR(col));
	mvaddch(posY, posX, ch);
	attroff(COLOR_PAIR(col));
}

// adds an item to the actor's inventory
void Actor::pickItem(int x, int y)
{
	// add item to inventory
	/*container->add(std::move(*this));*/
}

void Actor::equip(Actor& item)
{
	item.isEquipped = true;
	weaponEquipped = item.name;
}

void Actor::unequip(Actor& item)
{
	item.isEquipped = false;
	weaponEquipped = "None";
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
int Actor::get_distance(int tileX, int tileY) const noexcept
{
	// using chebyshev distance
	const int distance = std::max(abs(posX - tileX), abs(posY - tileY));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

// end of file: Actor.cpp
