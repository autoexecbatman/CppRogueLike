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
private:
	std::string roll;
public:
	Attacker(std::string roll);

	const std::string& get_roll() const noexcept { return roll; }
	void set_roll(const std::string& new_roll) noexcept { roll = new_roll; }
	void attack(Creature& attacker, Creature& target);
	void attack_with_dual_wield(Creature& attacker, Creature& target);
	void perform_single_attack(Creature& attacker, Creature& target, const std::string& damageRoll, int attackPenalty, const std::string& handName);
	
	void load(const json& j) override;
	void save(json& j) override;
};

#endif // !ATTACKER_H
// file: Attacker.h
