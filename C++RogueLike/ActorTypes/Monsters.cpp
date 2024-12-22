// file: Monsters.cpp
#include "Monsters.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMonster.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Healer.h"

//==GOBLIN==
ActorData goblinData
{
	'g',
	"goblin",
	GOBLIN_PAIR
};

Goblin::Goblin(Vector2D position) : Creature(position, goblinData)
{
	RandomDice d;
	const int hp = d.d8();
	const int thaco = 20;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Short Sword";

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 15, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

//==ORC==
ActorData orcData
{
	'o',
	"orc",
	ORC_PAIR
};

Orc::Orc(Vector2D position) : Creature(position, orcData)
{
	RandomDice d;
	const int damage = d.d10();
	const int hp = d.d10();
	const int thaco = 19;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Long Sword";

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead orc", 35, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

//==TROLL===
ActorData trollData
{
	'T',
	"troll",
	TROLL_PAIR
};

Troll::Troll(Vector2D position) : Creature(position, trollData)
{
	RandomDice d;
	const int damage = d.d10() + 3;
	const int dmgMin = 1;
	const int dmgMax = 13;
	const int hp = d.d12();
	const int thaco = 13;
	const int ac = 4;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead troll", 100, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

//==DRAGON===
ActorData dragonData
{
	'D',
	"dragon",
	DRAGON_PAIR
};

Dragon::Dragon(Vector2D position) : Creature(position, dragonData)
{
	RandomDice d;
	const int damage = d.d12() + 5;
	const int dmgMin = 1;
	const int dmgMax = 17;
	const int hp = d.d12() + 5;
	const int thaco = 9;
	const int ac = 1;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 2, "dead dragon", 200, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

ActorData shopkeeperData
{
	'S',
	"shopkeeper",
	WHITE_PAIR
};

//==SHOPKEEPER==
Shopkeeper::Shopkeeper(Vector2D position) : Creature(position, shopkeeperData)
{
	destructible = std::make_unique<MonsterDestructible>(10, 0, "dead shopkeeper", 10, 10, 10);
	attacker = std::make_unique<Attacker>("D10");
	ai = std::make_unique<AiShopkeeper>();
	container = std::make_unique<Container>(10);
	auto healthPotion = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '!', "health potion", HPBARMISSING_PAIR });
	healthPotion->pickable = std::make_unique<Healer>(4);
	container->add(std::move(healthPotion));
	auto dagger = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '/', "dagger", 1 });
	dagger->pickable = std::make_unique<Dagger>();
	container->add(std::move(dagger));
}

// end of file: Goblin.cpp