#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"

//====

Destructible::Destructible(
	int hpMax,
	int defense,
	std::string corpseName,
	int xp
) :
	hpMax(hpMax),
	hp(hpMax),
	defense(defense),
	corpseName(corpseName),
	xp(xp)
{
	
}

Destructible::~Destructible() {}

int Destructible::take_damage(Actor& owner, int damage)
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

void Destructible::die(Actor& owner)
{

	//transform the actor into a corpse!
	owner.ch = '%';
	owner.name = corpseName;
	owner.blocks = false;

	//make sure corpses are drawn before living actors
	engine.send_to_back(owner);
}

//====

// The function returns the amount of health point actually restored.
int Destructible::heal(int hpToHeal)
{
	Destructible::hp += hpToHeal;
	
	if (Destructible::hp > Destructible::hpMax)
	{
		hpToHeal -= Destructible::hp - Destructible::hpMax;
		Destructible::hp = Destructible::hpMax;
	}

	return hpToHeal;
}

void Destructible::load(TCODZip& zip)
{
	hpMax = zip.getInt();
	hp = zip.getInt();
	defense = zip.getInt();
	corpseName = _strdup(zip.getString());
}

void Destructible::save(TCODZip& zip)
{
	zip.putInt(hpMax);
	zip.putInt(hp);
	zip.putInt(defense);
	zip.putString(corpseName.c_str());
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

std::shared_ptr<Destructible> Destructible::create(TCODZip& zip)
{
	DestructibleType type = (DestructibleType)zip.getInt();
	/*Destructible* destructible = nullptr;*/
	std::shared_ptr<Destructible> destructible = nullptr;

	switch (type) 
	{
	case DestructibleType::MONSTER:
		/*destructible = new MonsterDestructible(0, 0, NULL, 0);*/
		// make unique
		destructible = std::make_shared<MonsterDestructible>(0, 0, "NULL", 0);
		break;
	case DestructibleType::PLAYER:
		/*destructible = new PlayerDestructible(0, 0, NULL, 0);*/
		destructible = std::make_shared<PlayerDestructible>(0, 0, "NULL", 0);
		break;
	}
	
	destructible->load(zip);
	
	return destructible;

	/*return nullptr;*/
}

//==PlayerDestructible==

PlayerDestructible::PlayerDestructible(
	int hpMax,
	int defense,
	/*const char* corpseName,*/
	std::string corpseName,
	int xp
) :
	Destructible(hpMax, defense, corpseName, xp)
{
}

void PlayerDestructible::die(Actor& owner)
{
	/*std::cout << "You died!\n" << std::endl;*/
	/*int y = getmaxy(stdscr);*/
	mvprintw(29, 0, "You died!\n", owner.name.c_str());
	Destructible::die(owner);
	engine.gameStatus = Engine::GameStatus::DEFEAT;
}

//==MonsterDestructible==

MonsterDestructible::MonsterDestructible(
	int hpMax,
	int defense,
	/*const char* corpseName,*/
	std::string corpseName,
	int xp
) :
	Destructible(hpMax, defense, corpseName, xp)
{}

void MonsterDestructible::die(Actor& owner)
{
	mvprintw(29,0,"%s is dead\n", owner.name.c_str());
	
	engine.gui->log_message(
		WHITE_PAIR,
		"%s is dead. You gain %d xp\n",
		owner.name.c_str(),
		xp
	);
	engine.player->destructible->xp += xp;

	Destructible::die(owner);
}