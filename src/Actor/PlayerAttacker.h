#pragma once

#include "Attacker.h"
#include "EquipmentSlot.h"

class Player;
class Creature;
struct GameContext;

// Concrete attack strategy for the player.
// Reads damage from equipped weapons at attack time.
// Handles dual-wield branching — kept here, not in the engine base.
class PlayerAttacker : public Attacker
{
private:
	Player& owner;

	DamageInfo compute_weapon_damage(EquipmentSlot slot) const;

public:
	explicit PlayerAttacker(Player& owner);

	void attack(Creature& target, GameContext& ctx) override;

	// Player has no base DamageInfo to serialize.
	void load(const json& j) override {}
	void save(json& j) override {}
};
