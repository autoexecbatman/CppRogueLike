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

struct Disguise {
	char ch;
	std::string name;
	int color;
};

class Mimic : public Creature
{
public:
    Mimic(Vector2D position);

    // Make these public so AiMimic can access them
    bool isDisguised = true;
    int revealDistance = 1;  // Reveal true form when player is this close
    int confusionDuration = 3; // Turns of confusion
    int itemsConsumed = 0;   // Track how many items this mimic has eaten

    // Storage for possible disguises
    std::vector<struct Disguise> possibleDisguises;
    // Define the Disguise struct

private:

    // Initialize disguises (called from constructor)
    void initDisguises();
};

#endif // MONSTERS_H
// end of file: Monsters.h
