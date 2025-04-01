// file: Monsters.h
#ifndef MONSTERS_H
#define MONSTERS_H

#include "../Actor/Actor.h"
#include "../Colors/Colors.h"

//==GOBLIN==
class Goblin : public Creature
{
public:
	Goblin(Vector2D position);
};
//====

//==ORC==
class Orc : public Creature
{
public:
	Orc(Vector2D position);
};
//====

//==TROLL===
class Troll : public Creature
{
public:
	Troll(Vector2D position);
};
//====

//==DRAGON===
class Dragon : public Creature
{
public:
	Dragon(Vector2D position);
};
//====

class Shopkeeper : public Creature
{
public:
	Shopkeeper(Vector2D position);
};

//==ARCHER==
class Archer : public Creature
{
public:
	Archer(Vector2D position);
};

//==MAGE==
class Mage : public Creature
{
public:
	Mage(Vector2D position);
};

#endif // MONSTERS_H
// end of file: Monsters.h
