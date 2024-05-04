// file: Goblin.h
#ifndef GOBLIN_H
#define GOBLIN_H

#include "../Actor/Actor.h"

//==GOBLIN==
class Goblin : public Actor
{
public:
	Goblin(int y, int x);
};
//====

//==ORC==
class Orc : public Actor
{
public:
	Orc(int y, int x);
};
//====

//==TROLL===
class Troll : public Actor
{
public:
	Troll(int y, int x);
};
//====

//==DRAGON===
class Dragon : public Actor
{
public:
	Dragon(int y, int x);
};
//====

#endif // GOBLIN_H
// end of file: Goblin.h
