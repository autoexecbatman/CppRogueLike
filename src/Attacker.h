#pragma once

#include <string_view>

#include "AttackKind.h"
#include "AttackResult.h"
#include "DamageInfo.h"
#include "Persistent.h"

class Creature;
class Item;
struct GameContext;

struct BackstabInfo
{
	bool isBackstab{ false };
	int hitBonus{ 0 };
	int damageMultiplier{ 1 };
};

// Abstract base - combat resolution engine shared by MonsterAttacker and PlayerAttacker.
// Subclasses supply the attacker identity (stored owner reference) and implement attack().
class Attacker : public Persistent
{
private:
	// Used by MonsterAttacker. PlayerAttacker ignores it.
	DamageInfo damageInfo;

protected:
	explicit Attacker(const DamageInfo& damage);

	// Shared combat resolution engine - called by both strategies.
	// DamageInfo is supplied by the caller (subclass), not computed here.
	AttackResult perform_single_attack(
		Creature& owner,
		Creature& target,
		const DamageInfo& attackDamage,
		int attackPenalty,
		std::string_view handName,
		AttackKind kind,
		GameContext& ctx);

	BackstabInfo calculate_backstab_bonus(const Creature& owner) const noexcept;

	// What a blow with this weapon rolls: its own dice with its enhancement's damage bonus.
	// No weapon, or something that is not one, strikes unarmed.
	//
	// Example:
	//   weapon_damage(compositeBow);   // -> 1d6
	//   weapon_damage(nullptr);        // -> the unarmed 1d2
	[[nodiscard]] static DamageInfo weapon_damage(const Item* weapon);

	// The weapon's own adjustment to hit (PHB p.88): a cursed one is -2 whatever bonus it
	// carries, otherwise its enhancement's toHitBonus. No weapon adjusts nothing.
	//
	// Example:
	//   weapon_hit_modifier(bowOfPlusTwo);   // -> 2
	//   weapon_hit_modifier(cursedBow);      // -> -2
	//   weapon_hit_modifier(nullptr);        // -> 0
	[[nodiscard]] static int weapon_hit_modifier(const Item* weapon);

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
		std::string_view handName,
		GameContext& ctx) const noexcept;

	void log_attack_miss(
		const Creature& attacker,
		const Creature& target,
		int attackRoll,
		int rollNeeded,
		int attackPenalty,
		std::string_view handName,
		GameContext& ctx) const noexcept;

public:
	// Subclasses implement attack() with their owner reference and damage source.
	// kind says which way the attack is being made; it decides which weapon is
	// used and whether the missile to-hit adjustment applies. What comes back is
	// what the attack did, which a bite carrying poison reads before injecting it.
	virtual AttackResult attack(Creature& target, AttackKind kind, GameContext& ctx) = 0;

	// DamageInfo accessors - valid for MonsterAttacker; PlayerAttacker leaves this empty.
	const DamageInfo& get_damage_info() const noexcept { return damageInfo; }

	void set_damage_info(const DamageInfo& damage) noexcept { damageInfo = damage; }

	// Serializes damageInfo - used by MonsterAttacker. PlayerAttacker overrides with no-ops.
	void load(const json& source) override;
	void save(json& output) override;
};

// What a fired missile shows, decided by how its attack came out.
enum class ShotImpact
{
	NOTHING, // nothing was loosed, so nothing flies
	MISSILE, // the missile flies and misses
	MISSILE_AND_BLOOD, // the missile flies and lands
};

namespace RangedShot
{

// The picture an attack's outcome earns: blood only where it landed, and no missile
// where none was loosed.
//
// Example:
//   RangedShot::impact_of(AttackResult::LANDED);      // -> ShotImpact::MISSILE_AND_BLOOD
//   RangedShot::impact_of(AttackResult::MISSED);      // -> ShotImpact::MISSILE
//   RangedShot::impact_of(AttackResult::PREVENTED);   // -> ShotImpact::NOTHING
[[nodiscard]] ShotImpact impact_of(AttackResult result) noexcept;

// One ranged attack by `shooter` at `target`, for the player and the monsters alike.
// The attack resolves first, so its messages print as the missile leaves, and the
// missile drawn after it is the one impact_of names. Refuses a shooter with no attacker.
//
// Example, an archer two squares from the player:
//   RangedShot::fire(archer, player, ctx);   // -> AttackResult::LANDED; a bolt, then blood
//   RangedShot::fire(archer, corpse, ctx);   // -> AttackResult::PREVENTED; nothing drawn
AttackResult fire(Creature& shooter, Creature& target, GameContext& ctx);

} // namespace RangedShot
