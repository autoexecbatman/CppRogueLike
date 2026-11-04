#pragma once

#include <nlohmann/json_fwd.hpp>

#include "AttackKind.h"
#include "AttackResult.h"
#include "Attacker.h"

namespace CppRL
{

	class Player;
	class Creature;
	struct GameContext;

	// Concrete attack strategy for the player.
	// Reads damage from equipped weapons at attack time.
	// Handles dual-wield branching - kept here, not in the engine base.
	class PlayerAttacker : public Attacker
	{
	private:
		Player& owner;

	public:
		explicit PlayerAttacker(Player& owner);

		AttackResult attack(
			Creature& target,
			AttackKind kind,
			GameContext& ctx) override;

		// Player has no base DamageInfo to serialize.
		void load(const nlohmann::json& source) override;
		void save(nlohmann::json& output) const override;
	};

} // namespace CppRL
