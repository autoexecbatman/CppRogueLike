#include <algorithm>
#include <cassert>
#include <string>

#include "Actor.h"
#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "MagicalItemEffects.h"
#include "MessageSystem.h"
#include "TileConfig.h" // for TileConfig type used in ctor
#include "Vector2D.h"
#include "Web.h"

Web::Web(Vector2D position, int strength, const TileConfig& tileConfig)
	: SpellTile(position, ActorData{ tileConfig.get("TILE_WEB"), "spider web", ColorPairId::BLACK_WHITE }),
	  webStrength(strength)
{
	// Webs don't block movement but do have their effect when passed through
	// Note that webs don't have the BLOCKS state
}

// Apply web effect when a creature tries to pass through
EntryResult Web::on_creature_enter(Creature& creature, GameContext& ctx)
{
	assert(ctx.dice && "Web::on_creature_enter: called without dice");

	// Spiders and their kin move across webs freely.
	if (creature.has_state(ActorState::CAN_WALK_WEBS))
	{
		return EntryResult::UNAFFECTED;
	}

	// Said to the player about the player, about a monster only when the player sees it.
	const bool isSeen = creature.is_player() || ctx.map->is_in_fov(creature.position);
	const auto narrate = [&](ColorPairId color, const std::string& playerLine, const std::string& watcherLine)
	{
		if (isSeen)
		{
			ctx.messageSystem->message(color, creature.is_player() ? playerLine : watcherLine, MessageCompletion::FINISHED);
		}
	};
	const std::string& name = creature.actorData.name;

	// Check for Ring of Free Action (AD&D 2e: grants immunity to webs and paralysis)
	if (creature.wears_ring_of(MagicalEffect::FREE_ACTION))
	{
		narrate(ColorPairId::CYAN_BLACK, "Your ring of free action protects you from the web!", "The web cannot hold the " + name + ".");
		destroy();
		return EntryResult::AFFECTED;
	}

	// Calculate chance to get caught based on dexterity and web strength
	int catchChance = 40 + (webStrength * 10) - ((creature.get_dexterity() - 10) * 3);
	catchChance = std::min(90, std::max(10, catchChance)); // Cap between 10-90%

	if (ctx.dice->d100() <= catchChance)
	{
		// Calculate number of turns stuck based on web strength
		int stuckTurns = webStrength + ctx.dice->roll(1, 2);

		// Apply the effect through polymorphic interface
		creature.apply_web_effect(stuckTurns, webStrength, this);

		narrate(ColorPairId::WHITE_BLACK, "You're caught in a sticky web!", "The " + name + " is caught in a sticky web!");

		// The step is refused and the step already paid for the turn; charging here
		// as well would cost two rounds for one pace.
		return EntryResult::BLOCKED;
	}
	else
	{
		narrate(ColorPairId::WHITE_BLACK, "You carefully navigate through the web.", "The " + name + " pushes through the web.");

		// 50% chance to destroy the web
		if (ctx.dice->d2() == 1)
		{
			destroy();
			narrate(ColorPairId::WHITE_BLACK, "You tear through the web, clearing a path.", "The " + name + " tears the web apart.");
		}

		return EntryResult::AFFECTED;
	}
}

// Destroy this web
void Web::destroy()
{
	mark_destroyed();
}
