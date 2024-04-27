// file: Destructible.cpp
#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"
#include "StrengthAttributes.h"

//====
Destructible::Destructible(int hpMax,int dr,std::string corpseName,int xp, int thaco, int armorClass)
	:
	hpMax(hpMax),
	hp(hpMax),
	dr(dr),
	corpseName(corpseName),
	xp(xp),
	thaco(thaco),
	armorClass(armorClass)
{}

Destructible::~Destructible() {}

void Destructible::take_damage(Actor& owner, int damage)
{
	// check if damage is greater than 0
	// if it is, then apply the damage to the actor
	if (damage > 0)
	{
		owner.destructible->hp -= damage;
		if (hp <= 0)
		{
			owner.destructible->die(owner);
		}
	}
	else
	{
		damage = 0;
	}
}

void Destructible::die(Actor& owner)
{

	//transform the actor into a corpse!
	owner.ch = '%';
	owner.name = corpseName;
	owner.blocks = false;

	//make sure corpses are drawn before living actors
	game.send_to_back(owner);
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
	dr = zip.getInt();
	corpseName = _strdup(zip.getString());
	xp = zip.getInt();
	thaco = zip.getInt();
	armorClass = zip.getInt();
}

void Destructible::save(TCODZip& zip)
{
	zip.putInt(hpMax);
	zip.putInt(hp);
	zip.putInt(dr);
	zip.putString(corpseName.c_str());
	zip.putInt(xp);
	zip.putInt(thaco);
	zip.putInt(armorClass);
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

std::unique_ptr<Destructible> Destructible::create(TCODZip& zip)
{
	DestructibleType type{ (DestructibleType)zip.getInt() };
	std::unique_ptr<Destructible> destructible{};

	switch (type)
	{
	case DestructibleType::MONSTER:
	{
		destructible = std::make_unique<MonsterDestructible>(0, 0, "", 0, 0, 0);
		break;
	}

	case DestructibleType::PLAYER:
	{
		destructible = std::make_unique<PlayerDestructible>(0, 0, "", 0, 0, 0);
		break;
	}
	default:
		break;
	}
	
	if (destructible) { destructible->load(zip); }
	
	return destructible;
}

//==PlayerDestructible==

PlayerDestructible::PlayerDestructible(
	int hpMax,
	int dr,
	std::string corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{
}

void PlayerDestructible::die(Actor& owner)
{
	/*std::cout << "You died!\n" << std::endl;*/
	/*int y = getmaxy(stdscr);*/
	mvprintw(29, 0, "You died!\n", owner.name.c_str());
	Destructible::die(owner);
	game.gameStatus = Game::GameStatus::DEFEAT;
}

//==MonsterDestructible==

MonsterDestructible::MonsterDestructible(
	int hpMax,
	int dr,
	std::string corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{}

void MonsterDestructible::die(Actor& owner)
{
	mvprintw(29,0,"%s is dead\n", owner.name.c_str());
	
	game.gui->log_message(
		WHITE_PAIR,
		"%s is dead. You gain %d xp\n",
		owner.name.c_str(),
		xp
	);
	game.player->destructible->xp += xp;

	Destructible::die(owner);
}

// end of file: Destructible.cpp
