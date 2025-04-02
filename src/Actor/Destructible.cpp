// file: Destructible.cpp
#include <iostream>
#include <string>
#include <algorithm>

#include "../Game.h"
#include "../Items.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Attributes/StrengthAttributes.h"
#include "../ActorTypes/Healer.h"
#include "../CorpseFood.h"

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

	// Replace this line:
	// corpse->pickable = std::make_unique<Healer>(10);
	// With this:
	corpse->pickable = std::make_unique<CorpseFood>(0); // 0 means calculate from type

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

void Destructible::load(const json& j)
{
	hpMax = j.at("hpMax").get<int>();
	hp = j.at("hp").get<int>();
	dr = j.at("dr").get<int>();
	corpseName = j.at("corpseName").get<std::string>();
	xp = j.at("xp").get<int>();
	thaco = j.at("thaco").get<int>();
	armorClass = j.at("armorClass").get<int>();
}

void Destructible::save(json& j)
{
	j["hpMax"] = hpMax;
	j["hp"] = hp;
	j["dr"] = dr;
	j["corpseName"] = corpseName;
	j["xp"] = xp;
	j["thaco"] = thaco;
	j["armorClass"] = armorClass;
}

void PlayerDestructible::save(json& j)
{
	j["type"] = static_cast<int>(DestructibleType::PLAYER);
	Destructible::save(j);
}

void MonsterDestructible::save(json& j) 
{
	j["type"] = static_cast<int>(DestructibleType::MONSTER);
	Destructible::save(j);
}

std::unique_ptr<Destructible> Destructible::create(const json& j)
{
	if (j.contains("type") && j["type"].is_number())
	{
		const auto type = static_cast<DestructibleType>(j["type"].get<int>());
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
			destructible->load(j);
		}
		return destructible;
	}
	return nullptr;
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
	game.player->ai->levelup_update(*game.player);

	Destructible::die(owner);
}

// end of file: Destructible.cpp
