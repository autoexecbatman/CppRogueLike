// file: Actor.h
#ifndef ACTOR_H
#define ACTOR_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include <gsl/gsl>

#include "../Persistent/Persistent.h"
#include "../Ai/Ai.h"
#include "../Attributes/StrengthAttributes.h"

#include "Attacker.h"
#include "Destructible.h"
#include "Container.h"
#include "Pickable.h"

// {y,x} position
struct Vector2D
{
	int y;
	int x;

	// operator overloads

	// add two vectors
	Vector2D operator+(const Vector2D& rhs) const noexcept
	{
		return { y + rhs.y, x + rhs.x };
	}

	// subtract two vectors
	Vector2D operator-(const Vector2D& rhs) const noexcept
	{
		return { y - rhs.y, x - rhs.x };
	}
};

struct ActorData
{
	char ch{ 'f' };
	std::string name{ "string" };
	int color{ 0 };
};

struct ActorFlags
{
	bool blocks;
	bool fovOnly;
	bool canSwim;
	bool isEquipped;
};

//==Actor==
// a class for the actors in the game
// (player, monsters, items, etc.)
class Actor : public Persistent
{
public:
	//==Actor Attributes==
	int strength{ 0 };
	int dexterity{ 0 };
	int constitution{ 0 };
	int intelligence{ 0 };
	int wisdom{ 0 };
	int charisma{ 0 };

	int playerLevel{ 1 };
	std::string gender{ "None" };
	std::string playerClass{ "None" };
	std::string playerRace{ "None" };

	std::string weaponEquipped{ "None" };
	int value{ 0 };

	Vector2D position{ 0,0 };
	ActorData actorData{ 0,"string",0 };
	ActorFlags flags{ true,true,true,true };
	
	std::unique_ptr<Attacker> attacker; // the actor can attack
	gsl::not_null<std::unique_ptr<Destructible>> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<Container> container; // the actor can be a container
	std::unique_ptr<Pickable> pickable; // the actor can be picked
	std::unique_ptr<StrengthAttributes> strengthAttributes; // the actor can have strength attributes
	
	Actor(Vector2D position, ActorData data, ActorFlags flags);
	virtual ~Actor();

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void update(); // update() will handle the monster turn.

	int get_tile_distance(Vector2D tilePosition) const noexcept; // a function to get the distance from an actor to a specific tile of the map

	void render() const noexcept; // render the actor on the screen.
	void pickItem(int x, int y); // pick up an item

	void equip(Actor& item);
	void unequip(Actor& item);

	int get_strength() const noexcept { return strength; } // get the strength of the actor
	int get_posY() const noexcept { return position.y; } // get the y position of the actor
	int get_posX() const noexcept { return position.x; } // get the x position of the actor
	Vector2D get_position() const noexcept { return position; } // get the position of the actor
	bool is_visible() const noexcept; // check if the actor is visible
};

#endif // !ACTOR_H
// end of file: Actor.h
