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

//==MIMIC==
struct Disguise {
    char ch;
    std::string name;
    int color;
};

class Mimic : public Creature
{
public:
    Mimic(Vector2D position);

    // Only expose what's necessary through getters
    std::vector<Disguise> get_possible_disguises() const;

private:
    // Initialize possible disguises
    void initDisguises();

    // Storage for possible disguises
    std::vector<struct Disguise> possibleDisguises;
};

#endif // MONSTERS_H
// end of file: Monsters.h
