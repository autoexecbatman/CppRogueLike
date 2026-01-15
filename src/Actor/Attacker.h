#pragma once

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "../Persistent/Persistent.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"

class Creature;
struct GameContext;

class Attacker : public Persistent
{
private:
	DamageInfo damageInfo; // Base damage for monsters

	void perform_single_attack(Creature& attacker, Creature& target, int attackPenalty, const std::string& handName, GameContext& ctx);

public:
	explicit Attacker(const DamageInfo& damage);

	// Get effective damage - uses equipped weapon for players, base damage for monsters
	DamageInfo get_attack_damage(Creature& attacker) const;

	// Direct damage access for monsters
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }
	void set_damage_info(const DamageInfo& damage) noexcept { damageInfo = damage; }
	int roll_damage() const { return damageInfo.roll_damage(); }

	// Main attack entry point - handles single and dual wield
	void attack(Creature& attacker, Creature& target, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
};
