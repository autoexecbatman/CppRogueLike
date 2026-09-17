#pragma once

#include <vector>

#include "DamageInfo.h"

class Creature;
struct GameContext;

namespace DamageResolver
{
	// Hit points that have been through reduce_dice for their type. Only the
	// resolver makes one, so a creature handed one knows its resistance was
	// applied; the plain-integer damage path refuses fire and cold for that
	// reason. The number may still be adjusted after - a save halves it, a
	// strength bonus and damage reduction move it - through at().
	//
	// Example:
	//   const ResistedDamage burn = reduce_dice({ 6, 6, 6 }, DamageType::FIRE, 1);
	//   burn.hit_points();       // -> 12
	//   burn.at(6).hit_points(); // -> 6, still fire, still resisted
	class ResistedDamage
	{
	private:
		DamageType type{ DamageType::PHYSICAL };
		int hitPoints{ 0 };

		ResistedDamage(DamageType damageType, int points) : type(damageType), hitPoints(points) {}
		friend ResistedDamage reduce_dice(const std::vector<int>& dice, DamageType damageType, int strength);

	public:
		[[nodiscard]] DamageType damage_type() const noexcept { return type; }
		[[nodiscard]] int hit_points() const noexcept { return hitPoints; }
		// The same resisted damage at a different number, never below zero.
		[[nodiscard]] ResistedDamage at(int points) const noexcept { return ResistedDamage{ type, points < 0 ? 0 : points }; }
	};

	// How strongly a creature resists fire or cold, in rings: 0 for none, 1 for
	// a ring of fire resistance, a ring of warmth or a drunk potion, 2 for the
	// helm of brilliance, which the Dungeon Master's Guide calls a double-strength
	// ring. The greatest of the worn and the drunk, never a sum: the book says
	// the helm's protection cannot be augmented by further magical means.
	// Zero for every other damage type.
	//
	// Example, wearing a ring of fire resistance:
	//   resistance_strength(DamageType::FIRE, wearer, ctx);   // -> 1
	//   resistance_strength(DamageType::COLD, wearer, ctx);   // -> 0
	int resistance_strength(DamageType damageType, const Creature& owner, GameContext& ctx);

	// The bonus a resisting creature adds to its saving throw against magical
	// fire or cold. Ring of Fire Resistance: "saved against with a +4 bonus to
	// the die roll"; Ring of Warmth: "a saving throw bonus of +2 versus
	// cold-based attacks". Scaled by strength, so the helm gives +8.
	//
	// Example:
	//   save_bonus_against(DamageType::FIRE, ringWearer, ctx);   // -> 4
	//   save_bonus_against(DamageType::FIRE, helmWearer, ctx);   // -> 8
	int save_bonus_against(DamageType damageType, const Creature& owner, GameContext& ctx);

	// The hit points a run of damage dice deals through a resistance of the given
	// strength: fire dice "are calculated at -2 per die, but each die is never
	// less than 1"; cold dice at -1 per die. A strength of 0 sums the dice as
	// rolled. Other damage types are not dice-resisted and are summed as rolled.
	//
	// Example:
	//   reduce_dice({ 6, 6, 6 }, DamageType::FIRE, 1).hit_points();   // -> 12, three fours
	//   reduce_dice({ 1, 1, 1 }, DamageType::FIRE, 2).hit_points();   // -> 3, never below one
	ResistedDamage reduce_dice(const std::vector<int>& dice, DamageType damageType, int strength);

	// Applies the percentage resistance buffs the book has not been read for -
	// lightning and poison. Fire and cold pass through unchanged: their
	// resistance is applied to the dice and the save at the source, by the
	// producers enumerated in the definition.
	// Returns actual damage after reduction.
	int apply_resistances(
		int damage,
		DamageType damageType,
		const Creature& owner,
		GameContext& ctx);

	// Apply temporary HP shield. Pure calculation: temp HP absorbs damage first.
	// Returns damage post-shield and updated tempHp value.
	ShieldResult apply_temp_hp_shield(int damage, int tempHp);
}
