// file: Monsters.cpp
#include "Monsters.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMonster.h"
#include "../Random/RandomDice.h"

//==GOBLIN==
ActorData goblinData
{
	'g',
	"goblin",
	GOBLIN_PAIR
};
ActorFlags goblinFlags
{
	true,
	true,
	false,
	false
};
Goblin::Goblin(Vector2D position) : Actor(position, goblinData, goblinFlags)
{
	RandomDice d;
	const int damage = d.d6();
	const int hp = d.d8();
	const int dmgMin = 1;
	const int dmgMax = 6;
	const int thaco = 20;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Short Sword";

	attacker = std::make_unique<Attacker>(damage, dmgMin, dmgMax);
	/*destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 15, thaco, ac);*/
	destructible->hp = hp;
	destructible->dr = 0;
	destructible->corpseName = "dead goblin";
	destructible->xp = 15;
	destructible->thaco = thaco;
	destructible->armorClass = ac;

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
ActorFlags orcFlags
{
	true,
	true,
	false,
	false
};
Orc::Orc(Vector2D position) : Actor(position, orcData, orcFlags)
{
	RandomDice d;
	const int damage = d.d10();
	const int dmgMin = 1;
	const int dmgMax = 10;
	const int hp = d.d10();
	const int thaco = 19;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Long Sword";

	attacker = std::make_unique<Attacker>(damage, dmgMin, dmgMax);
	/*destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead orc", 35, thaco, ac);*/
	destructible->hp = hp;
	destructible->dr = 0;
	destructible->corpseName = "dead orc";
	destructible->xp = 35;
	destructible->thaco = thaco;
	destructible->armorClass = ac;

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
ActorFlags trollFlags
{
	true,
	true,
	false,
	false
};
Troll::Troll(Vector2D position) : Actor(position, trollData, trollFlags)
{
	RandomDice d;
	const int damage = d.d10() + 3;
	const int dmgMin = 1;
	const int dmgMax = 13;
	const int hp = d.d12();
	const int thaco = 13;
	const int ac = 4;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>(damage, dmgMin, dmgMax);
	/*destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead troll", 100, thaco, ac);*/
	destructible->hp = hp;
	destructible->dr = 1;
	destructible->corpseName = "dead troll";
	destructible->xp = 100;
	destructible->thaco = thaco;
	destructible->armorClass = ac;

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
ActorFlags dragonFlags
{
	true,
	true,
	false,
	false
};
Dragon::Dragon(Vector2D position) : Actor(position, dragonData, dragonFlags)
{
	RandomDice d;
	const int damage = d.d12() + 5;
	const int dmgMin = 1;
	const int dmgMax = 17;
	const int hp = d.d12() + 5;
	const int thaco = 9;
	const int ac = 1;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>(damage, dmgMin, dmgMax);
	/*destructible = std::make_unique<MonsterDestructible>(hp, 2, "dead dragon", 200, thaco, ac);*/
	destructible->hp = hp;
	destructible->dr = 2;
	destructible->corpseName = "dead dragon";
	destructible->xp = 200;
	destructible->thaco = thaco;
	destructible->armorClass = ac;

	ai = std::make_unique<AiMonster>();
}
//====

// end of file: Goblin.cpp
