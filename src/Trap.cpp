// file: Trap.cpp
// Implementation of trap mechanics: searching, triggering, disarming, damage

#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "Creature.h"
#include "DamageInfo.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "Map.h"
#include "MessageSystem.h"
#include "Player.h"
#include "PlayerRaceState.h"
#include "RandomDice.h"
#include "ThiefSkills.h"
#include "TileConfig.h"
#include "Trap.h"

namespace
{
// The lowest d100 roll that sets a trap off on the thief working on it: the book
// gives the accident a band of its own, 96 to 100, rather than making every
// failure spring it.
constexpr int ACCIDENTAL_TRIGGER = 96;

// A dwarf finds a pit on 1-3 on 1d6 (PHB page 44).
constexpr int DWARF_PIT_FIND_CHANCE = 3;

// A search covers the tiles touching the searcher, at Chebyshev distance one. A stated
// deviation, with its reasons on TrapSearch::search_around.
constexpr int SEARCH_REACH = 1;

// A dwarf's look takes one round, a stated deviation: the book gives it no time.
constexpr int DWARF_SEARCH_ROUNDS = 1;

// The rounds a disarm attempt took, rolled after the attempt itself: "This also requires
// 1d10 rounds" (PHB page 87). Only an attempt that rolled takes them.
int roll_disarm_rounds(RandomDice& dice)
{
	return dice.d10();
}

// Whether this character's racial sense finds stonework traps.
bool is_dwarf(const Player& searcher)
{
	return searcher.playerRaceState == PlayerRaceState::DWARF;
}

// Whether a Find/Remove Traps score is one a thief can roll against. A score of
// nothing is a skill not yet bought up to a usable percentage, which the book treats as
// not having it.
bool has_find_traps(std::optional<int> skill)
{
	return skill.has_value() && skill.value() > 0;
}
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
		return DisarmResult::ALREADY_TRIED;
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

	// Hidden, found, or left armed by a failed disarm: it springs. Nothing notices a
	// trap underfoot; only a search finds one.
	state = TrapState::TRIGGERED;
	// A sprung trap shows its sprung face, and is visible whether or not the
	// creature ever spotted it.
	actorData.tile = sprungTile;
	remove_state(ActorState::IS_INVISIBLE);

	// Roll damage
	const int damage = roll_damage(*ctx.dice);

	// Apply damage to creature
	assert(ctx.messageSystem && "Trap::on_creature_enter: called without a messageSystem");
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
	if (creature.is_player())
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, std::format("You trigger the {} and take {} damage!", trapName, damage), MessageCompletion::FINISHED);
	}
	else if (ctx.map->is_in_fov(creature.position))
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, std::format("The {} triggers the {} and takes {} damage!", creature.actorData.name, trapName, damage), MessageCompletion::FINISHED);
	}

	creature.take_damage_and_check_death(damage, ctx, DamageType::PHYSICAL);

	// 50% chance trap is destroyed after triggering
	if (ctx.dice->d2() == 1)
	{
		destroy();
		assert(ctx.messageSystem && "Trap::on_creature_enter: called without a messageSystem");
		if (creature.is_player() || ctx.map->is_in_fov(position))
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The trap is destroyed.", MessageCompletion::FINISHED);
		}
	}

	// The step is refused, and the step is also what paid for the turn: both paths
	// that reach a feature charge the player themselves, so charging again here
	// would take two rounds off the clock for one pace.
	return EntryResult::BLOCKED;
}

TrapSearchResult Trap::search(Player& searcher, GameContext& ctx)
{
	assert(ctx.dice && "Trap::search: called without dice");

	// Only a hidden trap has anything to find.
	if (state != TrapState::HIDDEN)
	{
		return TrapSearchResult::NOT_HIDDEN;
	}

	// A pit is the dwarf's to find and the small traps the thief's; neither finds the
	// other's kind.
	const bool isPit = (type == TrapType::PIT);
	const std::optional<int> skill = searcher.thief_skill(ThiefSkill::FIND_REMOVE_TRAPS);
	const bool canFind = isPit ? is_dwarf(searcher) : has_find_traps(skill);
	if (!canFind)
	{
		return TrapSearchResult::NO_WAY_TO_FIND;
	}

	// One search per level on this trap, and the trap remembers which.
	const int level = searcher.get_creature_level();
	if (level <= searchedAtLevel)
	{
		return TrapSearchResult::ALREADY_SEARCHED;
	}
	searchedAtLevel = level;

	const bool isFound = isPit ? (ctx.dice->d6() <= DWARF_PIT_FIND_CHANCE) : (ctx.dice->d100() <= skill.value());
	if (!isFound)
	{
		return TrapSearchResult::MISSED;
	}

	state = TrapState::DETECTED;
	remove_state(ActorState::IS_INVISIBLE);
	return TrapSearchResult::FOUND;
}

std::optional<int> TrapSearch::search_around(Player& searcher, GameContext& ctx)
{
	assert(ctx.messageSystem && "TrapSearch::search_around: called without a messageSystem");
	assert(ctx.traps && "TrapSearch::search_around: called without traps");
	assert(ctx.dice && "TrapSearch::search_around: called without dice");

	// A character with no way to find any trap is told so, and loses no time on it.
	const bool isThief = has_find_traps(searcher.thief_skill(ThiefSkill::FIND_REMOVE_TRAPS));
	if (!isThief && !is_dwarf(searcher))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You don't know how to look for traps.", MessageCompletion::FINISHED);
		return std::nullopt;
	}

	bool isAnyFound{ false };
	for (const auto& trap : *ctx.traps)
	{
		assert(trap && "TrapSearch::search_around: traps list holds a null entry");
		if (trap->is_destroyed() || trap->position.chebyshev_distance_to(searcher.position) != SEARCH_REACH)
		{
			continue;
		}
		if (trap->search(searcher, ctx) == TrapSearchResult::FOUND)
		{
			isAnyFound = true;
			ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, std::format("You find a hidden {}.", trap->actorData.name), MessageCompletion::FINISHED);
		}
	}

	if (!isAnyFound)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You find no traps.", MessageCompletion::FINISHED);
	}

	// The thief's 1d10 rounds whenever the thief's skill was part of the look, else the
	// dwarf's one round.
	return isThief ? ctx.dice->d10() : DWARF_SEARCH_ROUNDS;
}

std::optional<int> TrapDisarm::disarm_at(Player& thief, Vector2D target, GameContext& ctx)
{
	assert(ctx.messageSystem && "TrapDisarm::disarm_at: called without a messageSystem");
	assert(ctx.traps && "TrapDisarm::disarm_at: called without traps");
	assert(ctx.dice && "TrapDisarm::disarm_at: called without dice");

	// The live trap on the target tile, if there is one.
	const auto is_live_trap_at_target = [target](const std::unique_ptr<Trap>& trap)
	{
		assert(trap && "TrapDisarm::disarm_at: traps list holds a null entry");
		return !trap->is_destroyed() && trap->position == target;
	};
	const auto found = std::ranges::find_if(*ctx.traps, is_live_trap_at_target);
	if (found == ctx.traps->end())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There is no trap there.", MessageCompletion::FINISHED);
		return std::nullopt;
	}

	// Only an attempt that rolled takes time; every refusal answers before the dice.
	switch ((*found)->attempt_disarm(thief, ctx))
	{
	case DisarmResult::NOT_DISARMABLE:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There is no trap there.", MessageCompletion::FINISHED);
		return std::nullopt;
	}
	case DisarmResult::NOT_VISIBLE:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You don't see a trap there.", MessageCompletion::FINISHED);
		return std::nullopt;
	}
	case DisarmResult::ALREADY_DISARMED:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "That trap is already disarmed.", MessageCompletion::FINISHED);
		return std::nullopt;
	}
	case DisarmResult::NO_SKILL:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You cannot work a trap.", MessageCompletion::FINISHED);
		return std::nullopt;
	}
	case DisarmResult::ALREADY_TRIED:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "This trap stays beyond you until you gain a level.", MessageCompletion::FINISHED);
		return std::nullopt;
	}
	case DisarmResult::BEYOND_SKILL:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "This trap is beyond you.", MessageCompletion::FINISHED);
		return roll_disarm_rounds(*ctx.dice);
	}
	case DisarmResult::DISARMED:
	{
		ctx.messageSystem->message(ColorPairId::GREEN_BLACK, "You successfully disarm the trap.", MessageCompletion::FINISHED);
		return roll_disarm_rounds(*ctx.dice);
	}
	case DisarmResult::TRIGGERED:
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "You trigger the trap while attempting to disarm it!", MessageCompletion::FINISHED);
		return roll_disarm_rounds(*ctx.dice);
	}
	}
	std::unreachable();
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

void Trap::save(json& j)
{
	Actor::save(j); // Call base class save

	j["type"] = static_cast<int>(type);
	j["state"] = static_cast<int>(state);
	j["disarmAttemptedAtLevel"] = disarmAttemptedAtLevel;
	j["searchedAtLevel"] = searchedAtLevel;
}

void Trap::load(const json& j)
{
	Actor::load(j); // Call base class load

	state = static_cast<TrapState>(j.at("state").get<int>());
	disarmAttemptedAtLevel = j.at("disarmAttemptedAtLevel").get<int>();
	searchedAtLevel = j.at("searchedAtLevel").get<int>();

	// The type is set by the constructor the caller used, so the damage dice, the name
	// and both tiles are already right. The base has just restored a saved tile and a
	// saved invisibility over the top of them, and the state is what decides those - so
	// they are worked out again here rather than trusted from the record.
	apply_state_appearance();
}

void Trap::apply_state_appearance()
{
	// Hidden is the only state that draws nothing; sprung and defused show the second
	// face. Derived rather than saved, so the picture cannot disagree with the state.
	const bool spent = (state == TrapState::TRIGGERED || state == TrapState::DISARMED);
	actorData.tile = spent ? sprungTile : armedTile;

	if (state == TrapState::HIDDEN)
	{
		add_state(ActorState::IS_INVISIBLE);
	}
	else
	{
		remove_state(ActorState::IS_INVISIBLE);
	}
}
