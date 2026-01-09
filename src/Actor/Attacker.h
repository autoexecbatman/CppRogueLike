// file: Attacker.h
#ifndef ATTACKER_H
#define ATTACKER_H

#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include "../Persistent/Persistent.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include <string>

class Creature;
struct GameContext;

class Attacker : public Persistent
{
private:
	// Modern damage system using DamageInfo for consistent calculations
	DamageInfo damageInfo;
	// Legacy roll string kept for save/load compatibility during transition
	std::string roll;
public:
	Attacker(std::string roll);
	Attacker(const DamageInfo& damage);

	// Modern damage accessors
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }
	void set_damage_info(const DamageInfo& new_damage) noexcept { damageInfo = new_damage; }

	// Legacy accessors for backwards compatibility (UI, save/load)
	const std::string& get_roll() const noexcept { return roll; }
	void set_roll(const std::string& new_roll) noexcept {
		roll = new_roll;
		damageInfo = parse_damage_from_roll_string(new_roll);
	}
	void attack(Creature& attacker, Creature& target, GameContext& ctx);
	void attack_with_dual_wield(Creature& attacker, Creature& target, GameContext& ctx);
	// Modern clean interface for single attacks
	void perform_single_attack(Creature& attacker, Creature& target, int attackPenalty, const std::string& handName, GameContext& ctx);

	// Legacy compatibility for string-based damage rolls
	void perform_single_attack(Creature& attacker, Creature& target, const std::string& damageRoll, int attackPenalty, const std::string& handName, GameContext& ctx);

	// Clean unified damage interface - determines the correct damage source and calculates enhanced damage
	DamageInfo get_attack_damage(Creature& attacker) const;

	// Roll damage using the unified damage system
	int roll_attack_damage(Creature& attacker) const { return get_attack_damage(attacker).roll_damage(); }

private:
	// Legacy compatibility methods - prefer get_attack_damage() for new code
	DamageInfo get_enhanced_weapon_damage(Creature& attacker) const;
	DamageInfo parse_damage_from_roll_string(const std::string& rollStr) const;

public:
	// Legacy damage accessor - prefer roll_attack_damage() for new code
	int roll_damage() const { return damageInfo.roll_damage(); }
	
	void load(const json& j) override;
	void save(json& j) override;
};

#endif // !ATTACKER_H
// file: Attacker.h
