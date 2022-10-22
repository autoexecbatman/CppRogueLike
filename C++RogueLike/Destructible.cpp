#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"

//====

Destructible::Destructible(
	float maxHp,
	float defense,
	const char* corpseName,
	int xp
) :
	maxHp(maxHp),
	hp(maxHp),
	defense(defense),
	corpseName(corpseName),
	xp(xp)
{
	this->corpseName = _strdup(corpseName);
	xp = 0;
}

Destructible::~Destructible()
{
	free((char*)corpseName);
}

float Destructible::take_damage(Actor* owner, float damage)
{
	damage -= Destructible::defense; // (dam - def)

	if (damage > 0) // if dam > 0
	{

		Destructible::hp -= damage; // current hp - damage
		
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
	owner->name = corpseName;
	owner->blocks = false;

	//make sure corpses are drawn before living actors
	engine.send_to_back(owner);

}

//====

// The function returns the amount of health point actually restored.
float Destructible::heal(float hpToHeal)
{
	Destructible::hp += hpToHeal;
	
	if (Destructible::hp > Destructible::maxHp)
	{
		hpToHeal -= Destructible::hp - Destructible::maxHp;
		Destructible::hp = Destructible::maxHp;
	}

	return hpToHeal;
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

//template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
//int putEnum(T value) { putInt(static_cast<int>(value)); }
//
//template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
//T getEnum() { return static_cast<T>(getInt()); }


//Usage:
//putEnum(DestructibleType::PLAYER);
//auto des_type = getEnum<DestructibleType>();

//template<typename Enum, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
//int putEnum(Enum value) { return putInt(static_cast<std::underlying_type_v<Enum>>(value)); }

void PlayerDestructible::save(TCODZip& zip)
{
	/*zip.putInt(static_cast<int>(DestructibleType::PLAYER));*/
	zip.putInt(static_cast<int>(DestructibleType::PLAYER));
	Destructible::save(zip);
}

void MonsterDestructible::save(TCODZip& zip) 
{
	 /*zip.putInt(static_cast<int>(DestructibleType::MONSTER));*/
	// convert DestructibleType to type int using std::underlying_type_v
	zip.putInt(static_cast<std::underlying_type_t<DestructibleType>>(DestructibleType::MONSTER));
	
	Destructible::save(zip);
}

Destructible* Destructible::create(TCODZip& zip) 
{
	DestructibleType type = (DestructibleType)zip.getInt();
	Destructible* destructible = nullptr;

	switch (type) 
	{
	case DestructibleType::MONSTER:
		destructible = new MonsterDestructible(0, 0, NULL, 0);
		break;
	case DestructibleType::PLAYER:
		destructible = new PlayerDestructible(0, 0, NULL, 0);
		break;
	}
	
	destructible->load(zip);
	
	return destructible;
}

//==PlayerDestructible==

PlayerDestructible::PlayerDestructible(
	float maxHp,
	float defense,
	const char* corpseName,
	int xp
) :
	Destructible(maxHp, defense, corpseName, xp)
{
}

void PlayerDestructible::die(Actor* owner)
{
	/*std::cout << "You died!\n" << std::endl;*/
	/*int y = getmaxy(stdscr);*/
	mvprintw(29, 0, "You died!\n", owner->name);
	Destructible::die(owner);
	engine.gameStatus = Engine::GameStatus::DEFEAT;
}

//==MonsterDestructible==

MonsterDestructible::MonsterDestructible(
	float maxHp,
	float defense,
	const char* corpseName,
	int xp
) :
	Destructible(maxHp, defense, corpseName, xp)
{}

void MonsterDestructible::die(Actor* owner)
{
	mvprintw(29,0,"%s is dead\n", owner->name);
	
	engine.gui->log_message(WHITE_PAIR, "%s is dead. You gain %d xp", owner->name, xp);
	engine.player->destructible->xp += xp;

	Destructible::die(owner);
}