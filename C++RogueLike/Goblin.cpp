#include "Game.h"
#include "Actor.h"
#include "Colors.h"

#include "Goblin.h"
#include "RandomDice.h"


//==GOBLIN==
Goblin::Goblin(int y, int x) : Actor(y, x, 'g', "goblin", 15, 0)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = 'g';
	name = "goblin";
	col = 15;
	blocks = true;
	fovOnly = true;
	/*attacker = new Attacker(Attacker(d.d6()));*/
	int damage = d.d6();
	attacker = std::make_shared<Attacker>(Attacker(damage));
	/*destructible = new MonsterDestructible(d.d8(), 0, "dead goblin", 15);*/
	int hp = d.d8();
	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 0, "dead goblin", 15));
	/*ai = new MonsterAi();*/
	ai = std::make_shared<MonsterAi>();
}

//Actor* Goblin::create_goblin(int y, int x)
//{
//	return new Goblin(y, x);
//}

// make a create_goblin function that returns a std::unique_ptr<Actor> instead of an Actor*
std::shared_ptr<Goblin> Goblin::create_goblin(int y, int x)
{
	auto goblin = std::make_shared<Goblin>(y, x);
	return goblin;
}



//====

////==ORC==
//Orc::Orc(int y, int x) : Actor(y, x, 'o', "orc", ORC_PAIR)
//{
//	RandomDice d;
//	posY = y;
//	posX = x;
//	ch = 'o';
//	name = "orc";
//	col = ORC_PAIR;
//	blocks = true;
//	fovOnly = true;
//	/*attacker = new Attacker(Attacker(d.d10()));*/
//	int damage = d.d10();
//	attacker = std::make_shared<Attacker>(Attacker(d.d10()));
//	/*destructible = new MonsterDestructible(d.d10(), 0, "dead orc", 35);*/
//	int hp = d.d10();
//	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 0, "dead orc", 35));
//	/*ai = new MonsterAi();*/
//	ai = std::make_shared<MonsterAi>();
//}
//
//Orc::~Orc()
//{}
//
//std::shared_ptr<Actor> Orc::create_orc(int y, int x)
//{
//	auto orc = std::make_shared<Actor>(y, x);
//	return orc;
//}
//
////====
//
////==TROLL===
//Troll::Troll(int y, int x) : Actor(y, x, 'T', "troll", TROLL_PAIR)
//{
//	RandomDice d;
//	posY = y;
//	posX = x;
//	ch = 'T';
//	name = "troll";
//	col = TROLL_PAIR;
//	blocks = true;
//	fovOnly = true;
//	/*attacker = new Attacker(Attacker(d.d10() + 3));*/
//	int damage = d.d10() + 3;
//	attacker = std::make_shared<Attacker>(Attacker(damage));
//	/*destructible = new MonsterDestructible(d.d12(), 1, "dead troll", 100);*/
//	int hp = d.d12();
//	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 1, "dead troll", 100));
//	/*ai = new MonsterAi();*/
//	ai = std::make_shared<MonsterAi>();
//}
//
//Troll::~Troll()
//{}
//
////Actor* Troll::create_troll(int y, int x)
////{
////	return new Troll(y, x);
////}
//
//std::shared_ptr<Actor> Troll::create_troll(int y, int x)
//{
//	auto troll = std::make_shared<Actor>(y, x);
//	return troll;
//}
//
////====
//
////==DRAGON===
//Dragon::Dragon(int y, int x) : Actor(y, x, 'D', "dragon", DRAGON_PAIR)
//{
//	RandomDice d;
//	posY = y;
//	posX = x;
//	ch = 'D';
//	name = "dragon";
//	col = DRAGON_PAIR;
//	blocks = true;
//	fovOnly = true;
//	/*attacker = new Attacker(Attacker(d.d12() + 5));*/
//	int damage = d.d12() + 5;
//	attacker = std::make_shared<Attacker>(Attacker(d.d12() + 5));
//	/*destructible = new MonsterDestructible(d.d12() + 5, 2, "dead dragon", 200);*/
//	int hp = d.d12() + 5;
//	destructible = std::make_shared<MonsterDestructible>(MonsterDestructible(hp, 2, "dead dragon", 200));
//	/*ai = new MonsterAi();*/
//	ai = std::make_shared<MonsterAi>();
//}
//
//Dragon::~Dragon()
//{}
//
////Actor* Dragon::create_dragon(int y, int x)
////{
////	return new Dragon(y, x);
////}
//
//std::shared_ptr<Actor> Dragon::create_dragon(int y, int x)
//{
//	auto dragon = std::make_shared<Actor>(y, x);
//	return dragon;
//}
//
////====