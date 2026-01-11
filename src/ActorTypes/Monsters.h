// file: Monsters.h
#ifndef MONSTERS_H
#define MONSTERS_H

#include "../Actor/Actor.h"
#include "../Colors/Colors.h"

struct GameContext;

//==GOBLIN==
class Goblin : public Creature
{
public:
	Goblin(Vector2D position, GameContext& ctx);
};
//====

//==ORC==
class Orc : public Creature
{
public:
	Orc(Vector2D position, GameContext& ctx);
};
//====

//==TROLL===
class Troll : public Creature
{
public:
	Troll(Vector2D position, GameContext& ctx);
};
//====

//==DRAGON===
class Dragon : public Creature
{
public:
	Dragon(Vector2D position, GameContext& ctx);
};
//====

//==ARCHER==
class Archer : public Creature
{
public:
	Archer(Vector2D position, GameContext& ctx);
};

//==MAGE==
class Mage : public Creature
{
public:
	Mage(Vector2D position, GameContext& ctx);
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
    Mimic(Vector2D position, GameContext& ctx);

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
