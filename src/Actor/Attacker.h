#pragma once

#include <string>

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "../Persistent/Persistent.h"
#include "../Combat/DamageInfo.h"

class Creature;
struct GameContext;

struct BackstabInfo
{
	bool isBackstab{ false };
	int hitBonus{ 0 };
	int damageMultiplier{ 1 };
};

class Attacker : public Persistent
{
private:
	DamageInfo damageInfo; // Base damage for monsters

	void perform_single_attack(
		Creature& attacker,
		Creature& target,
		int attackPenalty,
		const std::string& handName,
		GameContext& ctx
	);

	BackstabInfo calculate_backstab_bonus(Creature& attacker) const noexcept;

	int calculate_to_hit_roll(
		const Creature& attacker,
		const Creature& target,
		int attackPenalty,
		const BackstabInfo& backstab,
		GameContext& ctx
	) const noexcept;

	int calculate_damage_with_backstab(
		int damageRoll,
		int strengthBonus,
		const BackstabInfo& backstab,
		GameContext& ctx
	) const noexcept;

	void log_attack_hit(
		const Creature& attacker,
		const Creature& target,
		int attackRoll,
		int rollNeeded,
		int attackPenalty,
		int finalDamage,
		int damageRoll,
		const DamageInfo& attackDamage,
		int strengthBonus,
		int dr,
		const std::string& handName,
		GameContext& ctx
	) const noexcept;

	void log_attack_miss(
		const Creature& attacker,
		const Creature& target,
		int attackRoll,
		int rollNeeded,
		int attackPenalty,
		const std::string& handName,
		GameContext& ctx
	) const noexcept;

public:
	explicit Attacker(const DamageInfo& damage);

	// Get effective damage - uses equipped weapon for players, base damage for monsters
	DamageInfo get_attack_damage(Creature& attacker) const;

	// Direct damage access for monsters
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }
	void set_damage_info(const DamageInfo& damage) noexcept { damageInfo = damage; }
	int roll_damage(RandomDice* dice) const { return damageInfo.roll_damage(dice); }

	// Main attack entry point - handles single and dual wield
	void attack(Creature& attacker, Creature& target, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
};
