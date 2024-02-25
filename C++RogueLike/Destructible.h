// file Destructible.h
#ifndef DESTRUCTIBLE_H
#define DESTRUCTIBLE_H
#include <iostream>

#include "Persistent.h"

class Actor;

//====

class Destructible : public Persistent
{
public:
	int hpMax{ 0 }; // maximum health points
	int hp{ 0 }; // current health points
	int dr{ 0 }; // hit points deflected
	std::string corpseName{ "corpseName" }; // the actor's name once it is dead/destroyed
	int xp{ 0 }; // for awarding experience points
	int thaco{ 0 }; // to hit armor class 0
	int armorClass{ 0 }; // armor class
	
	int food{ 0 };
	int foodMax{ 0 };
	int needToSleep{ 0 };

	Destructible(int hpMax, int dr, std::string corpseName, int xp, int thaco, int armorClass);
	virtual ~Destructible();

	// is the actor dead? (returns true if hp is below or equal to 0)
	bool is_dead() noexcept { return hp <= 0; }

	//int take_damage(Actor* owner, int damage); // handles damage, owner attacked, returns (dam - def)
	int take_damage(Actor& owner, int damage); // handles damage, owner attacked, returns (dam - def)

	//virtual void die(Actor* owner); // handles death, owner killed
	virtual void die(Actor& owner) = 0; // handles death, owner killed

	int heal(int hpToHeal); // The function returns the amount of health point actually restored.

	void load(TCODZip& zip);
	void save(TCODZip& zip);
	/*static Destructible* create(TCODZip& zip);*/
	static std::shared_ptr<Destructible> create(TCODZip& zip);
protected:
	enum class DestructibleType : int
	{
		MONSTER, PLAYER
	};

};

//====

class MonsterDestructible : public Destructible
{
public:
	MonsterDestructible(
		int hpMax,
		int dr,
		std::string corpseName,
		int xp,
		int thaco,
		int armorClass
	);
	//====
	//handles death, owner killed
	/*void die(Actor* owner);*/
	void die(Actor& owner);
	void save(TCODZip& zip);
};

//====

class PlayerDestructible : public Destructible
{
public:
	PlayerDestructible(
		int hpMax,
		int dr,
		std::string corpseName,
		int xp,
		int thaco,
		int armorClass
	);
	//====
	//handles death, owner killed	
	/*void die(Actor* owner);*/
	void die(Actor& owner);
	void save(TCODZip& zip);
};

//====

#endif // !DESTRUCTIBLE_H
// end of file: Destructible.h
