#pragma once

#include "DamageInfo.h"

class Creature;
struct GameContext;

namespace DamageResolver
{
	// Apply resistance buffs based on damage type.
	// Logs messages if resistances reduce damage.
	// Returns actual damage after reduction by resistances.
	int apply_resistances(
		int damage,
		DamageType damageType,
		const Creature& owner,
		GameContext& ctx);

	// Apply temporary HP shield. Pure calculation: temp HP absorbs damage first.
	// Returns damage post-shield and updated tempHp value.
	ShieldResult apply_temp_hp_shield(int damage, int tempHp);
}
