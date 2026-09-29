// file: Trap.cpp
// Implementation of trap mechanics: detection, triggering, disarming, damage

#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <ranges>
#include <string>

#include "DamageInfo.h"
#include "DiceExpr.h"
#include "Player.h"
#include "ThiefSkills.h"
#include "Trap.h"
#include "GameContext.h"
#include "RandomDice.h"
#include "Creature.h"
#include "Map.h"
#include "TileConfig.h"
#include "MessageSystem.h"

// The lowest d100 roll that sets a trap off on the thief working on it: the book
// gives the accident a band of its own, 96 to 100, rather than making every
// failure spring it.
namespace
{
constexpr int ACCIDENTAL_TRIGGER = 96;
} // namespace

Trap::Trap(Vector2D position, TrapType trapType, const TileConfig& tileConfig)
	: TileFeature(position, ActorData{}),
	  type(trapType)
{
	// Set damage dice, display name and both tiles based on trap type. The
	// tiles are only ever seen once the trap is no longer hidden: a hidden trap
	// carries IS_INVISIBLE and draws nothing at all.
	std::string trapName;
	switch (type)
	{
	case TrapType::PIT:
	{
		damageDiceCount = 2;
		damageDiceSize = 6; // 2d6 damage
		trapName = "pit trap";
		armedTile = tileConfig.get("TILE_TRAP_PIT_ARMED");
		sprungTile = tileConfig.get("TILE_TRAP_PIT_SPRUNG");
		break;
	}
	case TrapType::DART:
	{
		damageDiceCount = 1;
		damageDiceSize = 4; // 1d4 damage
		trapName = "dart trap";
		armedTile = tileConfig.get("TILE_TRAP_DART_ARMED");
		sprungTile = tileConfig.get("TILE_TRAP_DART_SPRUNG");
		break;
	}
	case TrapType::ARROW:
	{
		damageDiceCount = 1;
		damageDiceSize = 6; // 1d6 damage
		trapName = "arrow trap";
		armedTile = tileConfig.get("TILE_TRAP_ARROW_ARMED");
		sprungTile = tileConfig.get("TILE_TRAP_ARROW_SPRUNG");
		break;
	}
	}

	actorData.name = trapName;
	actorData.tile = armedTile;
	actorData.color = ColorPairId::YELLOW_BLACK;

	// Hidden traps are invisible until detected
	add_state(ActorState::IS_INVISIBLE);
}

bool Trap::attempt_detect(Creature& creature, GameContext& ctx)
{
	if (state != TrapState::HIDDEN)
	{
		return false;  // Already detected, disarmed, or triggered
	}

	// Roll 1d20 + DEX modifier vs detection DC
	int roll = ctx.dice->roll(1, 20);
	int dexMod = (creature.get_dexterity() - 10) / 2;
	int checkResult = roll + dexMod;

	if (checkResult >= detectionDC)
	{
		state = TrapState::DETECTED;
		remove_state(ActorState::IS_INVISIBLE);  // Trap is now visible
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, "You notice a hidden trap!", MessageCompletion::FINISHED);
		}
		return true;
	}

	return false;
}

DisarmResult Trap::attempt_disarm(Player& creature, GameContext& ctx)
{
	// Nothing left to disarm.
	if (state == TrapState::DISARMED)
	{
		return DisarmResult::ALREADY_DISARMED;
	}

	// A trap the creature has not spotted cannot be worked on.
	if (state == TrapState::HIDDEN)
	{
		return DisarmResult::NOT_VISIBLE;
	}

	// Find/Remove Traps is the thief's, and the book gives nobody else a way to
	// work on a trap. A score of nothing is a skill not yet bought up to a usable
	// percentage, which the book treats the same way.
	const std::optional<int> skill = creature.thief_skill(ThiefSkill::FIND_REMOVE_TRAPS);
	if (!skill.has_value() || skill.value() <= 0)
	{
		return DisarmResult::NO_SKILL;
	}

	// "He can try disarming the trap again when he advances to the next experience
	// level": one attempt per level on this trap, and the trap remembers which.
	const int level = creature.get_creature_level();
	if (level <= disarmAttemptedAtLevel)
	{
		return DisarmResult::BEYOND_SKILL;
	}
	disarmAttemptedAtLevel = level;

	const int roll = ctx.dice->d100();
	if (roll <= skill.value())
	{
		state = TrapState::DISARMED;
		// A defused trap reads as a spent one: the pit is open and boarded over,
		// the darts are on the floor.
		actorData.tile = sprungTile;
		return DisarmResult::DISARMED;
	}

	// "If the dice roll is 96-100, the thief accidentally triggers the trap and
	// suffers the consequences."
	if (roll >= ACCIDENTAL_TRIGGER)
	{
		state = TrapState::TRIGGERED;
		on_creature_enter(creature, ctx);
		return DisarmResult::TRIGGERED;
	}

	// "the trap is beyond the thief's current skill" - so it is left as it was.
	return DisarmResult::BEYOND_SKILL;
}

EntryResult Trap::on_creature_enter(Creature& creature, GameContext& ctx)
{
	// A disarmed trap is walked over.
	if (state == TrapState::DISARMED)
	{
		return EntryResult::UNAFFECTED;
	}

	// The one detection roll a hidden trap gets: noticed now, the creature stops short.
	if (state == TrapState::HIDDEN)
	{
		attempt_passive_detection(creature, ctx);
		if (state == TrapState::DETECTED)
		{
			return EntryResult::BLOCKED;
		}
	}

	// Missed, already known, or left armed by a failed disarm: it springs.
	state = TrapState::TRIGGERED;
	// A sprung trap shows its sprung face, and is visible whether or not the
	// creature ever spotted it.
	actorData.tile = sprungTile;
	remove_state(ActorState::IS_INVISIBLE);

	// Roll damage
	const int damage = roll_damage(*ctx.dice);

	// Apply damage to creature
	if (ctx.messageSystem)
	{
		std::string trapName;
		switch (type)
		{
		case TrapType::PIT:
		{
			trapName = "pit";
			break;
		}
		case TrapType::DART:
		{
			trapName = "dart trap";
			break;
		}
		case TrapType::ARROW:
		{
			trapName = "arrow trap";
			break;
		}
		}
		ctx.messageSystem->message(ColorPairId::RED_BLACK, std::format("You trigger the {} and take {} damage!", trapName, damage), MessageCompletion::FINISHED);
	}

	creature.take_damage_and_check_death(damage, ctx, DamageType::PHYSICAL);

	// 50% chance trap is destroyed after triggering
	if (ctx.dice->d2() == 1)
	{
		destroy();
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The trap is destroyed.", MessageCompletion::FINISHED);
		}
	}

	// Trap blocks movement on first trigger (only if creature still alive)
	if (creature.get_hp() > 0)
	{
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
	}
	return EntryResult::BLOCKED;
}

void Trap::attempt_passive_detection(Creature& creature, GameContext& ctx)
{
	// Only a hidden trap is looked for; on_creature_enter asks for nothing else.
	assert(state == TrapState::HIDDEN && "passive detection on a trap that is not hidden");

	// Passive detection: roll vs detection DC
	const int roll = ctx.dice->roll(1, 20);
	const int dexMod = (creature.get_dexterity() - 10) / 2;
	const int checkResult = roll + dexMod;

	if (checkResult >= detectionDC)
	{
		state = TrapState::DETECTED;
		// Spotted is spotted however it was found: attempt_detect does the same,
		// and without this the message announces a trap that stays invisible.
		remove_state(ActorState::IS_INVISIBLE);
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, "You notice a hidden trap at the last moment!", MessageCompletion::FINISHED);
		}
	}
}

int Trap::roll_damage(RandomDice& dice) const
{
	// Every die rolled on its own: a 2d6 pit ranges from 2 to 12.
	return roll_dice(&dice, DiceExpr{ damageDiceCount, damageDiceSize, 0 });
}

void Trap::destroy()
{
	mark_destroyed();
}
