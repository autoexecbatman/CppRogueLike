#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"

//====

Destructible::Destructible(
	float maxHp,
	float defense,
	const char* corpseName
) :
	maxHp(maxHp),
	hp(maxHp),
	defense(defense),
	corpseName(corpseName)
{
	this->corpseName = _strdup(corpseName);
}

Destructible::~Destructible()
{
	free((char*)corpseName);
}

float Destructible::takeDamage(Actor* owner, float damage)
{
	damage -= defense; // (dam - def)
	if (damage > 0) // if dam > 0
	{
		hp -= damage; // current hp - damage
		if (hp <= 0) // if hp <= 0
		{
			die(owner); // owner killed
		}
	}
	else
	{
		damage = 0; // else 0 dam dealt
	}
	
	return damage; // total damage dealt
}

void Destructible::die(Actor* owner)
{
	//transform the actor into a corpse!
	owner->ch = '%';
	owner->col = DEAD_NPC_PAIR;
	owner->name = corpseName;
	owner->blocks = false;
	//make sure corpses are drawn before living actors
	engine.sendToBack(owner);
}

//====

MonsterDestructible::MonsterDestructible(
	float maxHp,
	float defense,
	const char* corpseName
) :
	Destructible(maxHp, defense, corpseName)
{}

void MonsterDestructible::die(Actor* owner)
{
	mvprintw(29,0,"%s is dead\n", owner->name);
	Destructible::die(owner);
}

//====

PlayerDestructible::PlayerDestructible(
	float maxHp,
	float defense,
	const char* corpseName
) :
	Destructible(maxHp, defense, corpseName)
{
}

void PlayerDestructible::die(Actor* owner)
{
	/*std::cout << "You died!\n" << std::endl;*/
	/*int y = getmaxy(stdscr);*/
	mvprintw(29, 0, "You died!\n", owner->name);
	Destructible::die(owner);
	engine.gameStatus = Engine::DEFEAT;
}

//====

// The function returns the amount of health point actually restored.
float Destructible::heal(float amount)
{
	// TODO: Add your implementation code here.
	hp += amount;
	if (hp > maxHp)
	{
		amount -= hp - maxHp;
		hp = maxHp;
	}
	return amount;
}

void Destructible::load(TCODZip& zip)
{
	maxHp = zip.getFloat();
	hp = zip.getFloat();
	defense = zip.getFloat();
	corpseName = _strdup(zip.getString());
}

void Destructible::save(TCODZip& zip)
{
	zip.putFloat(maxHp);
	zip.putFloat(hp);
	zip.putFloat(defense);
	zip.putString(corpseName);
}

void PlayerDestructible::save(TCODZip& zip)
{
	zip.putInt(PLAYER);
	Destructible::save(zip);
}

void MonsterDestructible::save(TCODZip& zip) 
{
	zip.putInt(MONSTER);
	Destructible::save(zip);
}

Destructible* Destructible::create(TCODZip& zip) {
	DestructibleType type = (DestructibleType)zip.getInt();
	Destructible* destructible = NULL;
	switch (type) {
	case MONSTER: destructible = new MonsterDestructible(0, 0, NULL); break;
	case PLAYER: destructible = new PlayerDestructible(0, 0, NULL); break;
	}
	destructible->load(zip);
	return destructible;
}