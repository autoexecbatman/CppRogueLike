// file: Attacker.h
#ifndef ATTACKER_H
#define ATTACKER_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include "../Persistent/Persistent.h"
#include "string_view"

class Creature;

class Attacker : public Persistent
{
public:
	std::string_view roll{ "diceroll" };
	Attacker(std::string_view roll);

	void attack(Creature& attacker, Creature& target);
	
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
private:
	int missCount{ 0 };
};

#endif // !ATTACKER_H
// file: Attacker.h
