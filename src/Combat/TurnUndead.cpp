#include <algorithm>

#include "../Actor/Creature.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "TurnUndead.h"
#include "TurningTable.h"

namespace
{
// Only clerics channel a deity. Paladins turn in the book at two levels lower,
// which this game has no class for yet.
bool can_turn_undead(const Creature& creature)
{
	if (!creature.is_player())
	{
		return false;
	}

	const Player& player = static_cast<const Player&>(creature);
	return player.playerClassState == Player::PlayerClassState::CLERIC;
}
} // namespace

TurnUndeadReport turn_undead(Creature& priest, GameContext& ctx)
{
	TurnUndeadReport report{};

	if (!can_turn_undead(priest))
	{
		return report;
	}

	report.attempted = true;

	// One roll for the whole attempt, read separately against each type.
	report.roll = ctx.dice->d20();

	// The weakest undead are affected first, which matters once the 2d6 cap bites.
	std::vector<Creature*> candidates{};
	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "creatures list holds a null entry");

		if (!creature->is_undead() || creature->is_dead())
		{
			continue;
		}

		if (priest.get_tile_distance(creature->position) > TURN_UNDEAD_RANGE)
		{
			continue;
		}

		candidates.push_back(creature.get());
	}

	std::ranges::sort(candidates,
		[](const Creature* left, const Creature* right)
		{ return left->get_hit_dice() < right->get_hit_dice(); });

	int affectedRemaining = ctx.dice->roll(TURN_UNDEAD_DICE_COUNT, TURN_UNDEAD_DICE_SIDES);

	for (Creature* undead : candidates)
	{
		const TurningResult result = look_up_turning(undead->get_hit_dice(), priest.get_creature_level());

		// Beyond this priest's power, or the roll fell short: no effect, and it
		// does not consume one of the 2d6.
		if (result.outcome == TurningOutcome::BEYOND_POWER
			|| (result.outcome == TurningOutcome::ROLL_REQUIRED && report.roll < result.rollNeeded))
		{
			report.resisted.push_back(undead);
			continue;
		}

		if (affectedRemaining <= 0)
		{
			report.resisted.push_back(undead);
			continue;
		}

		--affectedRemaining;

		if (result.outcome == TurningOutcome::DESTROYED)
		{
			undead->take_damage_and_check_death(undead->get_hp(), ctx);
			report.destroyed.push_back(undead);
			continue;
		}

		// Turned: the undead flees the priest's presence.
		undead->add_state(ActorState::IS_FLEEING);
		report.turned.push_back(undead);
	}

	return report;
}
