// file: Attacker.h
#ifndef ATTACKER_H
#define ATTACKER_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include "../Persistent/Persistent.h"

class Creature;

class Attacker : public Persistent
{
public:
	int dmg{ 0 };
	int maxDmg{ 0 };
	int minDmg{ 0 };
	std::string roll{ "diceroll" };
	

	Attacker(int dmg, int minDmg, int maxDmg) noexcept;

	void attack(Creature& attacker, Creature& target);
	
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
private:
	int missCount{ 0 };
};

#endif // !ATTACKER_H
// file: Attacker.h
