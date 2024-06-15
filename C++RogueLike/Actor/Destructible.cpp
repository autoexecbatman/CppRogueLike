// file: Destructible.cpp
#include <iostream>
#include <string>
#include <algorithm>

//#include "../main.h"
#include "../Game.h"
#include "../Items.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Attributes/StrengthAttributes.h"
#include "../ActorTypes/Healer.h"

//====
Destructible::Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass)
	:
	hpMax(hpMax),
	hp(hpMax),
	dr(dr),
	corpseName(corpseName),
	xp(xp),
	thaco(thaco),
	armorClass(armorClass)
{}

void Destructible::take_damage(Creature& owner, int damage)
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

// Transform the actor into a corpse !
void Destructible::die(Creature& owner)
{
	// copy data to new entity of type Item
	auto corpse = std::make_unique<Item>(owner.position, owner.actorData);
	corpse->actorData.name = corpseName;
	corpse->actorData.ch = '%';
	corpse->pickable = std::make_unique<Healer>(10);

	// remove the actor from the game
	std::erase_if(game.creatures, [&owner](const auto& c) { return c.get() == &owner; });

	// add the item to the game
	game.container->add(std::move(corpse));
}

//====

// The function returns the amount of health point actually restored.
int Destructible::heal(int hpToHeal)
{
	hp += hpToHeal;
	
	if (hp > hpMax)
	{
		hpToHeal -= hp - hpMax;
		hp = hpMax;
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
	
	if (destructible)
	{
		destructible->load(zip);
	}
	
	return destructible;
}

//==PlayerDestructible==

PlayerDestructible::PlayerDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{
}

void PlayerDestructible::die(Creature& owner)
{
	Destructible::die(owner);
	game.gameStatus = Game::GameStatus::DEFEAT;
}

//==MonsterDestructible==

MonsterDestructible::MonsterDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{}

void MonsterDestructible::die(Creature& owner)
{
	// message which monster is dead
	game.appendMessagePart(owner.actorData.color, std::format("{}", owner.actorData.name));
	game.appendMessagePart(WHITE_PAIR, " is dead.\n");
	game.finalizeMessage();
	
	// message how much xp you get
	game.appendMessagePart(WHITE_PAIR, "You get ");
	game.appendMessagePart(GOBLIN_PAIR, std::format("{}", xp));
	game.appendMessagePart(WHITE_PAIR, " experience points.\n");
	game.finalizeMessage();

	// increase the player's experience
	game.player->destructible->xp += xp;

	Destructible::die(owner);
}

// end of file: Destructible.cpp
