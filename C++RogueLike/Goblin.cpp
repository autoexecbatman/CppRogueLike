#include "Engine.h"
#include "Actor.h"
#include "Colors.h"

#include "Goblin.h"
#include "globals.h"


//==GOBLIN==
Goblin::Goblin(int y, int x) : Actor(y, x, 'g', "goblin", GOBLIN_PAIR)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = 'g';
	name = "goblin";
	col = GOBLIN_PAIR;
	blocks = true;
	fovOnly = true;
	attacker = new Attacker(Attacker(d.d6()));
	destructible = new MonsterDestructible(d.d8(), 0, "dead goblin", 15);
	ai = new MonsterAi();
}

Goblin::~Goblin() 
{}

//Actor* Goblin::create_goblin(int y, int x)
//{
//	return new Goblin(y, x);
//}

// make a create_goblin function that returns a std::unique_ptr<Actor> instead of an Actor*
Actor* Goblin::create_goblin(int y, int x)
{
	return new Goblin(y, x);
}



//====

//==ORC==
Orc::Orc(int y, int x) : Actor(y, x, 'o', "orc", ORC_PAIR)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = 'o';
	name = "orc";
	col = ORC_PAIR;
	blocks = true;
	fovOnly = true;
	attacker = new Attacker(Attacker(d.d10()));
	destructible = new MonsterDestructible(d.d10(), 0, "dead orc", 35);
	ai = new MonsterAi();
}

Orc::~Orc()
{}

Actor* Orc::create_orc(int y, int x)
{
	return new Orc(y, x);
}

//====

//==TROLL===
Troll::Troll(int y, int x) : Actor(y, x, 'T', "troll", TROLL_PAIR)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = 'T';
	name = "troll";
	col = TROLL_PAIR;
	blocks = true;
	fovOnly = true;
	attacker = new Attacker(Attacker(d.d10() + 3));
	destructible = new MonsterDestructible(d.d12(), 1, "dead troll", 100);
	ai = new MonsterAi();
}

Troll::~Troll()
{}

Actor* Troll::create_troll(int y, int x)
{
	return new Troll(y, x);
}

//====

//==DRAGON===
Dragon::Dragon(int y, int x) : Actor(y, x, 'D', "dragon", DRAGON_PAIR)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = 'D';
	name = "dragon";
	col = DRAGON_PAIR;
	blocks = true;
	fovOnly = true;
	attacker = new Attacker(Attacker(d.d12() + 5));
	destructible = new MonsterDestructible(d.d12() + 5, 2, "dead dragon", 200);
	ai = new MonsterAi();
}

Dragon::~Dragon()
{}

Actor* Dragon::create_dragon(int y, int x)
{
	return new Dragon(y, x);
}

//====