#pragma once

#include "Attacker.h"

class Creature;
struct GameContext;

// Concrete attack strategy for monsters.
// Owns a back-reference to its Creature owner (safe: Creature outlives its components).
// Uses stored DamageInfo for damage — inherited from Attacker base.
class MonsterAttacker : public Attacker
{
private:
	Creature& owner;

public:
	MonsterAttacker(Creature& owner, const DamageInfo& damage);

	void attack(Creature& target, GameContext& ctx) override;

	// load/save delegate to Attacker base (DamageInfo serialization).
	void load(const json& j) override { Attacker::load(j); }
	void save(json& j) override { Attacker::save(j); }
};
