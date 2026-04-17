#pragma once

#include <string>

#include "../Combat/DamageInfo.h"
#include "../Persistent/Persistent.h"
#include "../Random/RandomDice.h"

class Creature;
struct GameContext;

struct BackstabInfo
{
	bool isBackstab{ false };
	int hitBonus{ 0 };
	int damageMultiplier{ 1 };
};

// Abstract base — combat resolution engine shared by MonsterAttacker and PlayerAttacker.
// Subclasses supply the attacker identity (stored owner reference) and implement attack().
class Attacker : public Persistent
{
private:
	DamageInfo damageInfo; // Used by MonsterAttacker. PlayerAttacker ignores it.

protected:
	explicit Attacker(const DamageInfo& damage);

	// Shared combat resolution engine — called by both strategies.
	// DamageInfo is supplied by the caller (subclass), not computed here.
	void perform_single_attack(
		Creature& owner,
		Creature& target,
		const DamageInfo& attackDamage,
		int attackPenalty,
		const std::string& handName,
		GameContext& ctx);

	BackstabInfo calculate_backstab_bonus(const Creature& owner) const noexcept;

	int calculate_to_hit_roll(
		const Creature& attacker,
		const Creature& target,
		int attackPenalty,
		const BackstabInfo& backstab,
		GameContext& ctx) const noexcept;

	int calculate_damage_with_backstab(
		int damageRoll,
		int strengthBonus,
		const BackstabInfo& backstab,
		GameContext& ctx) const noexcept;

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
		GameContext& ctx) const noexcept;

	void log_attack_miss(
		const Creature& attacker,
		const Creature& target,
		int attackRoll,
		int rollNeeded,
		int attackPenalty,
		const std::string& handName,
		GameContext& ctx) const noexcept;

public:
	// Subclasses implement attack() with their owner reference and damage source.
	virtual void attack(Creature& target, GameContext& ctx) = 0;

	// DamageInfo accessors — valid for MonsterAttacker; PlayerAttacker leaves this empty.
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }
	void set_damage_info(const DamageInfo& damage) noexcept { damageInfo = damage; }
	int roll_damage(RandomDice* dice) const { return damageInfo.roll_damage(dice); }

	// Serializes damageInfo — used by MonsterAttacker. PlayerAttacker overrides with no-ops.
	void load(const json& j) override;
	void save(json& j) override;
};
