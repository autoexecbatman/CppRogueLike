// file: Destructible.cpp
#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"
#include "StrengthAttributes.h"

//====
Destructible::Destructible(int hpMax,int dr,std::string corpseName,int xp)
	:
	hpMax(hpMax),
	hp(hpMax),
	dr(dr),
	corpseName(corpseName),
	xp(xp)
{}

Destructible::~Destructible() {}

int Destructible::take_damage(Actor& owner, int damage)
{
	int str = owner.strength - 1; // -1 to access vector index from 0
	std::vector<StrengthAttributes> attrs = loadStrengthAttributes();
	if (str >= 0 && str < static_cast<int>(attrs.size())) {
		StrengthAttributes strength = attrs[str];
		damage = damage - Destructible::dr + strength.dmgAdj; // include dmgAdj
	}
	else {
		// Handle out of range case...
	}

	if (damage > 0)
	{
		Destructible::hp -= damage;
		if (hp <= 0)
		{
			die(owner);
		}
	}
	else
	{
		damage = 0;
	}

	//clear();
	//mvprintw(0, 0, "Damage: %d", damage);
	//refresh();
	//getch();

	return damage;
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
}

void Destructible::save(TCODZip& zip)
{
	zip.putInt(hpMax);
	zip.putInt(hp);
	zip.putInt(dr);
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
	DestructibleType type{ (DestructibleType)zip.getInt() };
	std::shared_ptr<Destructible> destructible{};

	switch (type)
	{
	case DestructibleType::MONSTER:
	{
		destructible = std::make_shared<MonsterDestructible>(0, 0, "", 0);
		break;
	}

	case DestructibleType::PLAYER:
	{
		destructible = std::make_shared<PlayerDestructible>(0, 0, "", 0);
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
	/*const char* corpseName,*/
	std::string corpseName,
	int xp
) :
	Destructible(hpMax, dr, corpseName, xp)
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
	/*const char* corpseName,*/
	std::string corpseName,
	int xp
) :
	Destructible(hpMax, dr, corpseName, xp)
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
