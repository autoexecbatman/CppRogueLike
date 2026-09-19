#pragma once

#include <string>

#include "AttackKind.h"
#include "DamageInfo.h"
#include "Persistent.h"

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
		AttackKind kind,
		GameContext& ctx);

	BackstabInfo calculate_backstab_bonus(const Creature& owner) const noexcept;

	// The armour class an attack on this target is rolled against: its own, or
	// without its Dexterity bonus while it is surprised.
	//
	// Example, armour class 6 of which Dexterity 18 gives 4:
	//   armor_class_attacked(player, ctx);   // -> 6
	//   armor_class_attacked(player, ctx);   // -> 10 while the player is IS_SURPRISED
	[[nodiscard]] int armor_class_attacked(const Creature& target, GameContext& ctx) const;

	// The d20 an attack needs. strengthHit is the part of Strength's hit adjustment
	// this attack takes, which AttackStrength::adjustment decides.
	int calculate_to_hit_roll(
		const Creature& attacker,
		const Creature& target,
		int attackPenalty,
		int strengthHit,
		const BackstabInfo& backstab,
		AttackKind kind,
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
	// kind says which way the attack is being made; it decides which weapon is
	// used and whether the missile to-hit adjustment applies.
	virtual void attack(Creature& target, AttackKind kind, GameContext& ctx) = 0;

	// DamageInfo accessors — valid for MonsterAttacker; PlayerAttacker leaves this empty.
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }
	void set_damage_info(const DamageInfo& damage) noexcept { damageInfo = damage; }

	// Serializes damageInfo — used by MonsterAttacker. PlayerAttacker overrides with no-ops.
	void load(const json& j) override;
	void save(json& j) override;
};
