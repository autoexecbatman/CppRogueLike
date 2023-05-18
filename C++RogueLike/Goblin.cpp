// file: Goblin.cpp
#include "Game.h"
#include "Actor.h"
#include "Colors.h"
#include "AiMonster.h"
#include "Goblin.h"
#include "RandomDice.h"

//==GOBLIN==
Goblin::Goblin(int y, int x) :
	Actor(y, x, 'g', "goblin", GOBLIN_PAIR, 0)
{
	RandomDice d;
	const int damage = d.d6();
	const int hp = d.d8();

	blocks = true;
	fovOnly = true;

	attacker = std::make_shared<Attacker>(damage);
	destructible = std::make_shared<MonsterDestructible>(hp, 0, "dead goblin", 15);
	ai = std::make_shared<AiMonster>();
}
//====

//==ORC==
Orc::Orc(int y, int x) :
	Actor(y, x, 'o', "orc", ORC_PAIR, 0)
{
	blocks = true;
	fovOnly = true;
	RandomDice d;
	const int damage = d.d10();
	attacker = std::make_shared<Attacker>(Attacker(damage));
	const int hp = d.d10();
	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 0, "dead orc", 35));
	ai = std::make_shared<AiMonster>();
}
//====

//==TROLL===
Troll::Troll(int y, int x) : Actor(y, x, 'T', "troll", TROLL_PAIR, 0)
{
	blocks = true;
	fovOnly = true;
	RandomDice d;
	const int damage = d.d10() + 3;
	attacker = std::make_shared<Attacker>(Attacker(damage));
	const int hp = d.d12();
	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 1, "dead troll", 100));
	ai = std::make_shared<AiMonster>();
}
//====

//==DRAGON===
Dragon::Dragon(int y, int x) : Actor(y, x, 'D', "dragon", DRAGON_PAIR, 100)
{
	blocks = true;
	fovOnly = true;
	RandomDice d;
	const int damage = d.d12() + 5;
	attacker = std::make_shared<Attacker>(Attacker(damage));
	const int hp = d.d12() + 5;
	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 2, "dead dragon", 200));
	ai = std::make_shared<AiMonster>();
}
//====

// end of file: Goblin.cpp
