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

class Mimic : public Creature
{
public:
    Mimic(Vector2D position);
    void update();
    void render() const noexcept;

private:
    bool isDisguised = true;
    int revealDistance = 2;  // Reveal true form when player is this close
    int confusionDuration = 5; // Turns of confusion

    int itemsConsumed = 0;   // Track how many items this mimic has eaten

    // Consume nearby items to grow stronger
    void consumeNearbyItems();

    // For disguise changes
    void changeDisguise();
    struct Disguise {
        char ch;
        std::string name;
        int color;
    };
    std::vector<Disguise> possibleDisguises = {
        {'$', "gold pile", GOLD_PAIR},
        {'!', "health potion", HPBARMISSING_PAIR},
        {'#', "scroll", LIGHTNING_PAIR},
        {'/', "weapon", WHITE_PAIR},
        {'%', "food", HPBARFULL_PAIR}
    };
    int disguiseChangeCounter = 0;
    static constexpr int disguiseChangeRate = 200; // How often to change disguise (in turns)
};

#endif // MONSTERS_H
// end of file: Monsters.h
