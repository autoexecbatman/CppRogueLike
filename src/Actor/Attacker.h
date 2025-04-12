// file: Attacker.h
#ifndef ATTACKER_H
#define ATTACKER_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include "../Persistent/Persistent.h"
#include <string>

class Creature;

class Attacker : public Persistent
{
public:
	std::string roll{ "diceroll" };
	Attacker(std::string roll);

	bool checkSurprise(Creature& attacker, Creature& target);

	void attack(Creature& attacker, Creature& target);
	
	void load(const json& j) override;
	void save(json& j) override;
};

#endif // !ATTACKER_H
// file: Attacker.h
