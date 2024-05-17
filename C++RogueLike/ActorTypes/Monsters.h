// file: Goblin.h
#ifndef GOBLIN_H
#define GOBLIN_H

#include "../Actor/Actor.h"
#include "../Colors/Colors.h"

//==GOBLIN==
class Goblin : public Actor
{
public:
	Goblin(Vector2D position);
};
//====

//==ORC==
class Orc : public Actor
{
public:
	Orc(Vector2D position);
};
//====

//==TROLL===
class Troll : public Actor
{
public:
	Troll(Vector2D position);
};
//====

//==DRAGON===
class Dragon : public Actor
{
public:
	Dragon(Vector2D position);
};
//====

#endif // GOBLIN_H
// end of file: Goblin.h
