#pragma once

#include "Attacker.h"

class Creature;
struct GameContext;

// Concrete attack strategy for monsters.
// Owns a back-reference to its Creature owner (safe: Creature outlives its components).
// Strikes in melee with its stored DamageInfo; shoots with its missile weapon's damage.
class MonsterAttacker : public Attacker
{
private:
	Creature& owner;

public:
	MonsterAttacker(Creature& owner, const DamageInfo& damage);

	AttackResult attack(
		Creature& target,
		AttackKind kind,
		GameContext& ctx) override;

	// load/save delegate to Attacker base (DamageInfo serialization).
	void load(const json& source) override { Attacker::load(source); }

	void save(json& output) override { Attacker::save(output); }
};
