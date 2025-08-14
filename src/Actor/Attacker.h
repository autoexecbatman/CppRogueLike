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

	void attack(Creature& attacker, Creature& target);
	void attack_with_dual_wield(Creature& attacker, Creature& target);
	void perform_single_attack(Creature& attacker, Creature& target, const std::string& damageRoll, int attackPenalty, const std::string& handName);
	
	void load(const json& j) override;
	void save(json& j) override;
};

#endif // !ATTACKER_H
// file: Attacker.h
