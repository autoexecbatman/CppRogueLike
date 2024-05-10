// file: Actor.h
#ifndef ACTOR_H
#define ACTOR_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "../Persistent/Persistent.h"
#include "../Ai/Ai.h"
#include "../Attributes/StrengthAttributes.h"

#include "Attacker.h"
#include "Destructible.h"
#include "Container.h"
#include "Pickable.h"

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

	int posY{ 0 }, posX{ 0 }; // position on map
	char ch{ -47 }; // the symbol to print
	int col{ 0 }; // color for the actor
	std::string name{ "actor_name" }; // add name
	bool blocks{ false }; // does the actor blocks movement?
	bool fovOnly{ false }; // to make some actors visible when not in fov
	bool canSwim{ false }; // can the actor swim?
	bool isEquipped{ false }; // is the actor equipped?
	int index{ 0 }; // index of the actor in the actors array
	/*bool sent_to_back = false;*/
	
	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<Destructible> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<Container> container; // the actor can be a container
	std::unique_ptr<Pickable> pickable; // the actor can be picked
	std::unique_ptr<StrengthAttributes> strengthAttributes; // the actor can have strength attributes
	
	Actor(
		int y,
		int x,
		int ch,
		std::string name,
		int col,
		int index
	);

	virtual ~Actor();

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void update(); // update() will handle the monster turn.

	int get_distance(int tileX, int tileY) const noexcept; // a function to get the distance from an actor to a specific tile of the map

	void render() const noexcept; // render the actor on the screen.
	void pickItem(int x, int y); // pick up an item

	void equip(Actor& item);
	void unequip(Actor& item);

	int get_strength() const noexcept { return strength; } // get the strength of the actor
	int get_posY() const noexcept { return posY; } // get the y position of the actor
	int get_posX() const noexcept { return posX; } // get the x position of the actor
	bool is_visible() const noexcept; // check if the actor is visible
};

#endif // !ACTOR_H
// end of file: Actor.h
