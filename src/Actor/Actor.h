// file: Actor.h
#ifndef ACTOR_H
#define ACTOR_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include <ranges>

#include "../Persistent/Persistent.h"
#include "../Ai/Ai.h"
#include "../Attributes/StrengthAttributes.h"

#include "Attacker.h"
#include "Destructible.h"
#include "Container.h"
#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Vector2D.h"

struct ActorData
{
	char ch{ 'f' };
	std::string name{ "string" };
	int color{ 0 };
};

enum class ActorState
{
	BLOCKS,
	FOV_ONLY,
	CAN_SWIM,
	IS_EQUIPPED,
	IS_RANGED,
	IS_CONFUSED,
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

	std::vector<ActorState> states;
	// C++ Core Guidelines F.6: noexcept for simple state operations
	bool has_state(ActorState state) const noexcept { return std::ranges::find(states, state) != states.end(); }
	void add_state(ActorState state) noexcept { states.push_back(state); }
	void remove_state(ActorState state) noexcept { std::erase_if(states, [state](ActorState s) { return s == state; }); }

	Actor(Vector2D position, ActorData data);
	virtual ~Actor() = default;

	void load(const json& j) override;
	void save(json& j) override;

	int get_tile_distance(Vector2D tilePosition) const noexcept;
	void render() const noexcept;
	bool is_visible() const noexcept;
};

class Creature : public Actor
{
public:
	Creature(Vector2D position, ActorData data) : Actor(position, data)
	{
		add_state(ActorState::BLOCKS);
		/*add_state(ActorState::FOV_ONLY);*/
	};

	void load(const json& j) override;
	void save(json& j) override;

	void update();

	//==Actor Attributes==
	int strength{ 0 };
	int dexterity{ 0 };
	int constitution{ 0 };
	int intelligence{ 0 };
	int wisdom{ 0 };
	int charisma{ 0 };

	int playerLevel{ 1 };
	int gold{ 0 };
	std::string gender{ "None" };
	std::string weaponEquipped{ "None" };

	void equip(Item& item);
	void unequip(Item& item);
	void syncRangedState();
	void pick();
	void drop(Item& item);

	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<Destructible> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<Container> container; // the actor can be a container
};

class NPC : public Creature
{
	public:
	NPC(Vector2D position, ActorData data) : Creature(position, data) {};
};

class Object : public Actor
{
public:
	Object(Vector2D position, ActorData data) : Actor(position, data) {};
};

class Item : public Object
{
public:
	Item(Vector2D position, ActorData data);

	void load(const json& j) override;
	void save(json& j) override;

	int value{ 1 };

	std::unique_ptr<Pickable> pickable; // the actor can be picked
};

class Stairs : public Object
{
	public:
	Stairs(Vector2D position) : Object(position, ActorData{ '>', "stairs", WHITE_BLACK_PAIR }) 
	{
		/*add_state(ActorState::FOV_ONLY);*/
	};
};

#endif // !ACTOR_H
// end of file: Actor.h
