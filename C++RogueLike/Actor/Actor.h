// file: Actor.h
#ifndef ACTOR_H
#define ACTOR_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include <gsl/gsl>
#include <ranges>
#include <bitset>

#include "../Persistent/Persistent.h"
#include "../Ai/Ai.h"
#include "../Attributes/StrengthAttributes.h"

#include "Attacker.h"
#include "Destructible.h"
#include "Container.h"
#include "Pickable.h"
#include "../Colors/Colors.h"

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

	// add two vectors
	Vector2D operator+=(const Vector2D& rhs) noexcept
	{
		y += rhs.y;
		x += rhs.x;
		return *this;
	}

	// subtract two vectors
	Vector2D operator-(const Vector2D& rhs) const noexcept
	{
		return { y - rhs.y, x - rhs.x };
	}

	// compare two vectors
	bool operator==(const Vector2D& rhs) const noexcept
	{
		return y == rhs.y && x == rhs.x;
	}

	// compare two vectors
	bool operator!=(const Vector2D& rhs) const noexcept
	{
		return !(*this == rhs);
	}

	// compare two vectors
	bool operator<(const Vector2D& rhs) const noexcept
	{
		return y < rhs.y || (y == rhs.y && x < rhs.x);
	}

	// compare two vectors
	bool operator>(const Vector2D& rhs) const noexcept
	{
		return y > rhs.y || (y == rhs.y && x > rhs.x);
	}

	// compare two vectors
	bool operator<=(const Vector2D& rhs) const noexcept
	{
		return *this < rhs || *this == rhs;
	}

	// compare two vectors
	bool operator>=(const Vector2D& rhs) const noexcept
	{
		return *this > rhs || *this == rhs;
	}

	// for bool conversion
	operator bool() const noexcept
	{
		return y != 0 || x != 0;
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
};

enum class ActorState
{
	BLOCKS,
	FOV_ONLY,
	CAN_SWIM,
	IS_EQUIPPED
};

//==Actor==
// a class for the actors in the game
// (player, monsters, items, etc.)
class Actor : public Persistent
{
public:
	Vector2D position{ 0,0 };
	Vector2D direction{ 0,0 };
	ActorData actorData{ 0,"string",0 };
	ActorFlags flags{ true,true,true };

	std::vector<ActorState> states;
	bool has_state(ActorState state) const noexcept { return std::ranges::find(states, state) != states.end(); }
	void add_state(ActorState state) noexcept { states.push_back(state); }
	void remove_state(ActorState state) noexcept { std::erase_if(states, [state](ActorState s) { return s == state; }); }

	Actor(Vector2D position, ActorData data, ActorFlags flags);
	virtual ~Actor() = default;

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	int get_tile_distance(Vector2D tilePosition) const noexcept;
	void render() const noexcept;
	bool is_visible() const noexcept;
};

class Creature : public Actor
{
public:
	Creature(Vector2D position, ActorData data, ActorFlags flags) : Actor(position, data, flags)
	{
		add_state(ActorState::BLOCKS);
		add_state(ActorState::FOV_ONLY);
	};

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void update(); // update() will handle the monster turn.

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

	void equip(Item& item);
	void unequip(Item& item);
	void pick();
	void drop();

	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<Destructible> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<Container> container; // the actor can be a container
};

class NPC : public Creature
{
	public:
	NPC(Vector2D position, ActorData data, ActorFlags flags) : Creature(position, data, flags) {};
};

class Object : public Actor
{
public:
	Object(Vector2D position, ActorData data, ActorFlags flags) : Actor(position, data, flags) {};
};

class Item : public Object
{
public:
	Item(Vector2D position, ActorData data, ActorFlags flags) : Object(position, data, flags) {};

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	int value{ 0 };

	std::unique_ptr<Pickable> pickable; // the actor can be picked
};

class Stairs : public Object
{
	public:
	Stairs(Vector2D position) : Object(position, ActorData{ '>', "stairs", WHITE_PAIR }, { .blocks = true, .fovOnly = true, .canSwim = true }) 
	{
		add_state(ActorState::FOV_ONLY);
	};
};

#endif // !ACTOR_H
// end of file: Actor.h
