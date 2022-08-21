#include <iostream>
#include <algorithm>

#include "main.h"
#include "Colors.h"

//====

Destructible::Destructible(
	float maxHp,
	float defense,
	const char* corpseName
) :
	maxHp(maxHp),
	hp(maxHp),
	defense(defense),
	corpseName(corpseName)
{}

float Destructible::takeDamage(Actor* owner, float damage)
{
    damage -= defense; // (dam - def)
    if (damage > 0) // if dam > 0
    {
        hp -= damage; // current hp - damage
        if (hp <= 0) // if hp <= 0
        {
            die(owner); // owner killed
        }
    }
    else
    {
        damage = 0; // else 0 dam dealt
    }
	
    return damage; // total damage dealt
}

void Destructible::die(Actor* owner)
{
	//transform the actor into a corpse!
    owner->ch = '%';
    owner->col = DEAD_NPC_PAIR;
	owner->name = corpseName;
	owner->blocks = false;
	//make sure corpses are drawn before living actors
    engine.sendToBack(owner);
}

//====

MonsterDestructible::MonsterDestructible(
    float maxHp,
    float defense,
    const char* corpseName
) :
    Destructible(maxHp, defense, corpseName)
{}

void MonsterDestructible::die(Actor* owner)
{
    mvprintw(29,0,"%s is dead\n", owner->name);
	Destructible::die(owner);
}

//====

PlayerDestructible::PlayerDestructible(
    float maxHp,
    float defense,
    const char* corpseName
) :
    Destructible(maxHp, defense, corpseName)
{
}

void PlayerDestructible::die(Actor* owner)
{
	/*std::cout << "You died!\n" << std::endl;*/
    /*int y = getmaxy(stdscr);*/
    mvprintw(29, 0, "You died!\n", owner->name);
	Destructible::die(owner);
    engine.gameStatus = Engine::DEFEAT;
}

//====