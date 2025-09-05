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
#include "../Utils/UniqueId.h"

#include "Attacker.h"
#include "Destructible.h"
#include "Container.h"
#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Utils/Vector2D.h"

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
	UniqueId::IdType uniqueId;

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
private:
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

public:
	Creature(Vector2D position, ActorData data) : Actor(position, data)
	{
		add_state(ActorState::BLOCKS);
		/*add_state(ActorState::FOV_ONLY);*/
	};

	void load(const json& j) override;
	void save(json& j) override;

	void update();

	// Const-correct getter methods
	int get_strength() const noexcept { return strength; }
	int get_dexterity() const noexcept { return dexterity; }
	int get_constitution() const noexcept { return constitution; }
	int get_intelligence() const noexcept { return intelligence; }
	int get_wisdom() const noexcept { return wisdom; }
	int get_charisma() const noexcept { return charisma; }
	int get_player_level() const noexcept { return playerLevel; }
	int get_gold() const noexcept { return gold; }
	const std::string& get_gender() const noexcept { return gender; }
	const std::string& get_weapon_equipped() const noexcept { return weaponEquipped; }

	// Setter methods
	void set_strength(int value) noexcept { strength = value; }
	void set_dexterity(int value) noexcept { dexterity = value; }
	void set_constitution(int value) noexcept { constitution = value; }
	void set_intelligence(int value) noexcept { intelligence = value; }
	void set_wisdom(int value) noexcept { wisdom = value; }
	void set_charisma(int value) noexcept { charisma = value; }
	void set_player_level(int value) noexcept { playerLevel = value; }
	void set_gold(int value) noexcept { gold = value; }
	void set_gender(const std::string& new_gender) noexcept { gender = new_gender; }
	void set_weapon_equipped(const std::string& weapon) noexcept { weaponEquipped = weapon; }
	
	// Modifier methods for increment/decrement operations
	void adjust_strength(int delta) noexcept { strength += delta; }
	void adjust_dexterity(int delta) noexcept { dexterity += delta; }
	void adjust_constitution(int delta) noexcept { constitution += delta; }
	void adjust_intelligence(int delta) noexcept { intelligence += delta; }
	void adjust_wisdom(int delta) noexcept { wisdom += delta; }
	void adjust_charisma(int delta) noexcept { charisma += delta; }
	void adjust_gold(int delta) noexcept { gold += delta; }

	void equip(Item& item);
	void unequip(Item& item);
	void sync_ranged_state();
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
