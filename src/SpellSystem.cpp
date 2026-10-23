// file: SpellSystem.cpp
//
// Every spell the game can cast, and the two shapes a spell comes in.
//
// An instant spell does its work and returns whether it happened -
// cast_magic_missile, cast_hold_person, cast_sleep. A targeted spell opens a
// TargetingMenu and returns immediately, doing its work in a callback when the
// player confirms a tile - cast_silence, cast_web, cast_fireball. Nothing here
// blocks; a menu that waited would hang the browser under Emscripten.
//
// Almost nothing calls these directly. cast_spell_by_key is the player's entry point: it
// looks the key up in the registry, checks the source, and dispatches. cast_spell_at is a
// monster's: the same checks, aimed at a creature the caller names instead of chosen.
//
// Usage:
//
//   const auto on_spent = [](GameContext& spentCtx) {}; // runs on success
//   SpellSystem::cast_spell_by_key("magic_missile", caster, SpellSource::MEMORIZED, on_spent, ctx);
//   SpellSystem::cast_spell_by_key("web", caster, SpellSource::ITEM, on_spent, ctx);
//   // the web's cursor opens, and on_spent fires on confirm
//   SpellSystem::cast_spell_at("fireball", lich, SpellSource::MEMORIZED, player, on_spent, ctx);
//   // bursts on the player's tile at once
//
//   SpellSystem::burst_fireball(center, 3, 2, ctx);             // -> { 3, 18, 1 }
//                                                               // public so a test skips the cursor
//
// Every loop over ctx.creatures here asserts the entry is non-null before touching
// it. The list owns what it holds, so a null is a fault rather than a case to skip,
// and folding that into the is-it-dead test is what hid it until 2026-10-01.

#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Actor.h"
#include "AnimationSystem.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "Colors.h"
#include "CreatureManager.h"
#include "DamageInfo.h"
#include "DamageResolver.h"
#include "DataManager.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "MagicalItemEffects.h"
#include "Map.h"
#include "MenuSpellCast.h"
#include "MessageSystem.h"
#include "Pickable.h"
#include "Player.h"
#include "RandomDice.h"
#include "SavingThrow.h"
#include "SpawnUtils.h"
#include "SpellAnimations.h"
#include "SpellRegistry.h"
#include "SpellSystem.h"
#include "TargetingMenu.h"
#include "TileConfig.h"
#include "TurnSchedule.h"
#include "Vector2D.h"
#include "Wording.h"

namespace
{
void animate_heal(const Vector2D& pos, GameContext& ctx)
{
	assert(ctx.tileConfig && "animate_heal: called without a tileConfig");
	assert(ctx.animSystem && "SpellSystem::animate_heal: called without an animSystem");

	ctx.animSystem->spawn_effect(
		pos,
		ctx.tileConfig->get("TILE_EFFECT_HEAL"),
		60,
		220,
		60,
		0.5f);
}

// AD&D 2e: 1 missile at level 1, +1 every 2 levels, max 5
int calculate_num_missiles(int casterLevel)
{
	return std::min(5, 1 + (casterLevel - 1) / 2);
}

// The spell list a class casts from; the classes that cast none answer NONE.
CasterClass to_caster_class(CreatureClass creatureClass)
{
	switch (creatureClass)
	{

	case CreatureClass::CLERIC:
	{
		return CasterClass::CLERIC;
	}

	case CreatureClass::WIZARD:
	{
		return CasterClass::WIZARD;
	}

	case CreatureClass::FIGHTER:
	case CreatureClass::ROGUE:
	case CreatureClass::MONSTER:
	{
		return CasterClass::NONE;
	}
	}

	std::unreachable();
}

// Table 24's sixth and seventh columns are footnoted: a priest reaches them only at
// these Wisdom scores. The rows are counted from one, so the fifth column is five
// rows and the sixth is six.
constexpr std::size_t SIXTH_SPELL_LEVEL_ROW = 5;
constexpr std::size_t SEVENTH_SPELL_LEVEL_ROW = 6;
constexpr int WISDOM_FOR_SIXTH_LEVEL = 17;
constexpr int WISDOM_FOR_SEVENTH_LEVEL = 18;

// The table's row for that level. A caster past the last printed row keeps what the
// last row gave it, which is where the book stops rather than a number of our own.
const std::vector<int>& row_for_level(const std::vector<std::vector<int>>& table, int level)
{
	static const std::vector<int> none{};
	if (level < 1)
	{
		return none;
	}
	const std::size_t row = std::min(static_cast<std::size_t>(level), table.size()) - 1;
	return table.at(row);
}
} // namespace

namespace Spells
{

int roll_sleep_hit_dice_budget(RandomDice& dice)
{
	// Each die on its own. roll() takes a range, so passing the count as its minimum
	// gave a flat 2 to 4 rather than 2d4.
	int total = 0;
	for (int die = 0; die < SLEEP_HIT_DICE_COUNT; ++die)
	{
		total += dice.roll(1, SLEEP_HIT_DICE_SIDES);
	}
	return total;
}

} // namespace Spells

namespace SpellSystem
{
namespace
{

// The player's choice of target, made the way each spell has always made it.
struct PlayerChooses
{
};

// A target the caller names, which a monster's cast passes.
struct AimedAt
{
	// The creature struck, or whose tile a burst centres on.
	Creature& target;
};

// Who a spell lands on.
using SpellTargets = std::variant<PlayerChooses, AimedAt>;

// AD&D 2e: a fireball's 20-foot radius is two tiles, and its range ten tiles plus one a level.
constexpr int FIREBALL_RADIUS = 2;

// How far a caster of that level can place a fireball, in tiles.
//
// Example:
//   fireball_range(18);   // -> 28
int fireball_range(int casterLevel)
{
	return 10 + casterLevel;
}

// Reached only through cast_spell_by_key and cast_spell_at, so nothing outside this file can call them.
void dispatch_effect(
	SpellEffectType effect,
	Creature& caster,
	SpellSource source,
	const SpellTargets& targets,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx);

// Every check a casting makes before its effect: silence, then a priest's fizzle. Shared by
// both entry points, so a monster is refused and fizzles exactly as the player is.
void cast_with_targets(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	const SpellTargets& targets,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx);

// Spell effect implementations (instant - return true on success)
bool cast_cure_light_wounds(Creature& caster, GameContext& ctx);
bool cast_bless(Creature& caster, GameContext& ctx);
bool cast_sanctuary(Creature& caster, GameContext& ctx);
bool cast_protection_from_evil(Creature& caster, GameContext& ctx);
// The player's choice of missile targets: the living creatures in the player's field of
// view, then fires at them. Refuses with a message and returns false when nothing is in sight.
//
// Example, caster level 3, one goblin in view:
//   cast_magic_missile(caster, ctx);   // -> true, 2 missiles, both to the goblin
//   cast_magic_missile(caster, ctx);   // -> false when no target is in view
bool cast_magic_missile(Creature& caster, GameContext& ctx);
// Fires one missile per two caster levels, five at most, at the targets nearest the caster
// first. A target whose Sanctuary turns the caster away is skipped and the missile goes to
// the next. Leaves the dead to the caller.
//
// Example, caster level 18, the player the one target:
//   resolve_magic_missile(lich, { &player }, ctx);   // 5 missiles, all to the player
void resolve_magic_missile(Creature& caster, std::vector<Creature*> targets, GameContext& ctx);
bool cast_shield(Creature& caster, GameContext& ctx);
bool cast_sleep(Creature& caster, GameContext& ctx);
// Makes the caster invisible for the spell's day, as the given buff, so a worn item's
// grant can be ended apart from the spell's.
bool cast_invisibility(Creature& caster, BuffType granted, GameContext& ctx);
bool cast_teleport(Creature& caster, GameContext& ctx);
bool cast_knock(Creature& caster, GameContext& ctx);
// Paralyzes up to 1d4 humanoids in the caster's field of view for two rounds
// per caster level. Each target saves versus spells, d20 at 15 or better, and a
// save negates for that target only. The 1d4 is the cap on targets attempted,
// not on targets held, so a roll of four against four saves holds nobody.
//
// Example, caster level 3, two orcs in view, 1d4 rolling 2, saves of 3 and 18:
//   cast_hold_person(caster, ctx);   // -> true, the first orc held for 6 rounds
bool cast_hold_person(Creature& caster, GameContext& ctx);

// Targeted spell implementations - async via TargetingMenu; onSuccess fires on confirm
// Opens a targeting cursor with a range of 5 plus the caster's level, and on
// confirmation silences the one living creature standing on the chosen tile for
// two rounds per caster level, preventing it from casting. Returns immediately:
// the work happens when the menu resolves, and onSuccess fires only then. A
// cancelled cursor says so and calls nothing.
//
// Example, caster level 3:
//   cast_silence(caster, on_spell_spent, ctx);   // cursor opens, range 8
//                                                // on confirm: 6 rounds, on cancel: nothing
void cast_silence(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
// Opens a targeting cursor with a range of five times the caster's level, and on
// confirmation entangles every living creature within two tiles of the chosen
// centre for two rounds per caster level. Each saves versus paralyzation, d20 at
// 10 or better, to escape. Returns immediately, as cast_silence does.
//
// Example, caster level 3, two goblins within the radius:
//   cast_web(caster, on_spell_spent, ctx);   // cursor opens, range 15, radius 2
//                                            // on confirm: each goblin rolls to escape
void cast_web(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
void cast_fireball(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
// Bursts a fireball of the caster level on center: the animation, the burst and its message.
// Leaves the dead to the caller.
//
// Example, caster level 13 on the player's tile:
//   resolve_fireball(player.position, 13, ctx);   // 10d6, the dice cap, "Fireball! 10d6 = ..."
void resolve_fireball(Vector2D center, int casterLevel, GameContext& ctx);

void dispatch_effect(
	SpellEffectType effect,
	Creature& caster,
	SpellSource source,
	const SpellTargets& targets,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// A named target skips the player's choice and lands at once.
	if (const AimedAt* aimed = std::get_if<AimedAt>(&targets))
	{
		assert((effect == SpellEffectType::MAGIC_MISSILE || effect == SpellEffectType::FIREBALL) && "dispatch_effect: a spell that cannot be aimed was aimed");
		if (effect == SpellEffectType::MAGIC_MISSILE)
		{
			resolve_magic_missile(caster, { &aimed->target }, ctx);
		}
		else
		{
			resolve_fireball(aimed->target.position, caster_level(caster), ctx);
		}
		onSuccess(ctx);
		return;
	}

	// Targeted spells: async via TargetingMenu - onSuccess is forwarded into the callback
	switch (effect)
	{

	case SpellEffectType::SILENCE:
	{
		cast_silence(caster, std::move(onSuccess), ctx);
		return;
	}

	case SpellEffectType::WEB:
	{
		cast_web(caster, std::move(onSuccess), ctx);
		return;
	}

	case SpellEffectType::FIREBALL:
	{
		cast_fireball(caster, std::move(onSuccess), ctx);
		return;
	}

	default:
	{
		break;
	}
	}

	// Instant spells: cast synchronously, call onSuccess if successful
	bool result = false;
	switch (effect)
	{

	case SpellEffectType::CURE_LIGHT_WOUNDS:
	{
		result = cast_cure_light_wounds(caster, ctx);
		break;
	}

	case SpellEffectType::BLESS:
	{
		result = cast_bless(caster, ctx);
		break;
	}

	case SpellEffectType::SANCTUARY:
	{
		result = cast_sanctuary(caster, ctx);
		break;
	}

	case SpellEffectType::PROTECTION_FROM_EVIL:
	{
		result = cast_protection_from_evil(caster, ctx);
		break;
	}

	case SpellEffectType::HOLD_PERSON:
	{
		result = cast_hold_person(caster, ctx);
		break;
	}

	case SpellEffectType::MAGIC_MISSILE:
	{
		result = cast_magic_missile(caster, ctx);
		break;
	}

	case SpellEffectType::SHIELD:
	{
		result = cast_shield(caster, ctx);
		break;
	}

	case SpellEffectType::SLEEP:
	{
		result = cast_sleep(caster, ctx);
		break;
	}

	case SpellEffectType::INVISIBILITY:
	{
		// An item's casting is the item's own buff, which taking the item off ends.
		const BuffType granted = source == SpellSource::ITEM ? BuffType::ITEM_INVISIBILITY : BuffType::INVISIBILITY;
		result = cast_invisibility(caster, granted, ctx);
		break;
	}

	case SpellEffectType::TELEPORT:
	{
		result = cast_teleport(caster, ctx);
		break;
	}

	case SpellEffectType::KNOCK:
	{
		result = cast_knock(caster, ctx);
		break;
	}

	default:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Spell not implemented yet.", MessageCompletion::FINISHED);
		break;
	}
	}

	if (result)
	{
		onSuccess(ctx);
	}
}

bool cast_cure_light_wounds(Creature& caster, GameContext& ctx)
{
	assert(ctx.dice && "SpellSystem::cast_cure_light_wounds: called without dice");

	animate_heal(caster.position, ctx);

	// Magical healing, which reaches the fire and acid wounds regeneration cannot.
	const int actualHealing = caster.heal(ctx.dice->roll(1, 8));

	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Cure Light Wounds! ");
	ctx.messageSystem->append_message_part(ColorPairId::GREEN_BLACK, std::format("+{} HP", actualHealing));
	ctx.messageSystem->finalize_message();

	return true;
}

bool cast_bless(Creature& caster, GameContext& ctx)
{
	// AD&D 2e, PHB page 426: 6 rounds.
	const int duration = 6;

	ctx.buffSystem->add_buff(caster, BuffType::BLESS, 0, duration, BuffApplication::ADD, ctx.gameState->get_time());
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Bless! ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format("+1 to hit for {}.", Wording::rounds(duration)));
	ctx.messageSystem->finalize_message();
	return true;
}

// AD&D 2e, Player's Handbook page 277: evil creatures attacking the protected
// suffer -2 on attack rolls. Duration is 2 rounds per caster level.
//
// Only the to-hit penalty is implemented. The spell's other two effects -
// blocking mental control, and hedging out conjured creatures - have nothing
// in this game to act on yet.
bool cast_protection_from_evil(Creature& caster, GameContext& ctx)
{
	const int duration = 2 * caster_level(caster);

	ctx.buffSystem->add_buff(caster, BuffType::PROTECTION_FROM_EVIL, PROTECTION_FROM_EVIL_PENALTY, duration, BuffApplication::ADD, ctx.gameState->get_time());

	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Protection From Evil! ");
	ctx.messageSystem->append_message_part(
		ColorPairId::WHITE_BLACK,
		std::format("Evil creatures strike at {} against you for {}.",
			PROTECTION_FROM_EVIL_PENALTY,
			Wording::rounds(duration)));
	ctx.messageSystem->finalize_message();
	return true;
}

bool cast_sanctuary(Creature& caster, GameContext& ctx)
{
	// AD&D 2e, PHB page 436: 2 rounds + 1 round/level. Cancelled by attacking.
	const int casterLevel = caster_level(caster);
	const int duration = 2 + casterLevel;

	ctx.buffSystem->add_buff(caster, BuffType::SANCTUARY, 0, duration, BuffApplication::ADD, ctx.gameState->get_time());

	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Sanctuary! ");
	ctx.messageSystem->append_message_part(
		ColorPairId::WHITE_BLACK,
		std::format("Divine protection shields you for {}.", Wording::rounds(duration)));
	ctx.messageSystem->finalize_message();
	return true;
}

void cast_silence(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Duration 2 rounds/level. Targeted single creature. Prevents spellcasting.
	int casterLevel = caster_level(caster);
	int duration = 2 * casterLevel;
	int range = 5 + casterLevel;

	auto onTarget = [duration, onSuccess = std::move(onSuccess)](
						bool confirmed,
						Vector2D targetPos,
						GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "Silence cancelled.", MessageCompletion::FINISHED);
			return;
		}

		Creature* target = nullptr;
		for (const auto& creature : *innerCtx.creatures)
		{
			assert(creature && "cast_silence: creatures list holds a null entry");

			if (creature->position == targetPos && !creature->is_dead())
			{
				target = creature.get();
				break;
			}
		}

		if (!target)
		{
			innerCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "No creature at that location.", MessageCompletion::FINISHED);
			return;
		}

		innerCtx.buffSystem->add_buff(*target, BuffType::SILENCE, 0, duration, BuffApplication::ADD, innerCtx.gameState->get_time());
		SpellAnimations::animate_creature_hit(target->position, innerCtx);

		innerCtx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Silence! ");
		innerCtx.messageSystem->append_message_part(
			ColorPairId::WHITE_BLACK,
			std::format("{} is struck mute for {}.", target->get_name(), Wording::rounds(duration)));
		innerCtx.messageSystem->finalize_message();

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, 0, std::move(onTarget), ctx));
}

void cast_web(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Save vs. Paralyzation (d20 >= 10) or be entangled. Duration 2 rounds/level.
	int casterLevel = caster_level(caster);
	int range = 5 * casterLevel;
	int radius = 2;
	int duration = 2 * casterLevel;

	auto onTarget = [radius, duration, onSuccess = std::move(onSuccess)](
						bool confirmed,
						Vector2D center,
						GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "Web cancelled.", MessageCompletion::FINISHED);
			return;
		}

		int affected = 0;
		for (const auto& creature : *innerCtx.creatures)
		{
			assert(creature && "cast_web: creatures list holds a null entry");

			if (creature->is_dead())
			{
				continue;
			}
			if (creature->get_tile_distance(center) > radius)
			{
				continue;
			}

			// AD&D 2e: a save versus paralyzation avoids the entanglement.
			if (!SavingThrows::is_made(*creature, SavingThrow::PARALYZATION_POISON_DEATH, 0, innerCtx))
			{
				innerCtx.buffSystem->add_buff(*creature, BuffType::WEBBED, 0, duration, BuffApplication::ADD, innerCtx.gameState->get_time());
				SpellAnimations::animate_creature_hit(creature->position, innerCtx);
				++affected;
			}
		}

		if (affected > 0)
		{
			innerCtx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Web! ");
			innerCtx.messageSystem->append_message_part(
				ColorPairId::WHITE_BLACK,
				std::format(
					"{} creature{} entangled for {}.",
					affected,
					affected == 1 ? " is" : "s are",
					Wording::rounds(duration)));
			innerCtx.messageSystem->finalize_message();
		}
		else
		{
			innerCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "The webs spread but catch nothing.", MessageCompletion::FINISHED);
		}

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, radius, std::move(onTarget), ctx));
}

void cast_fireball(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	const int casterLevel = caster_level(caster);

	auto onTarget = [casterLevel, onSuccess = std::move(onSuccess)](
						bool confirmed,
						Vector2D center,
						GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "Fireball cancelled.", MessageCompletion::FINISHED);
			return;
		}

		resolve_fireball(center, casterLevel, innerCtx);
		innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);
		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(fireball_range(casterLevel), FIREBALL_RADIUS, std::move(onTarget), ctx));
}

void resolve_fireball(Vector2D center, int casterLevel, GameContext& ctx)
{
	SpellAnimations::animate_explosion(center, FIREBALL_RADIUS, ctx);

	const FireballBurst burst = burst_fireball(center, casterLevel, FIREBALL_RADIUS, ctx);

	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, "Fireball! ");
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, std::format("{}d6 = {} damage", burst.diceCount, burst.totalDamage));
	if (burst.struck > 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" ({} struck)", burst.struck));
	}
	ctx.messageSystem->finalize_message();
}

bool cast_magic_missile(Creature& caster, GameContext& ctx)
{
	assert(ctx.creatureManager && "SpellSystem::cast_magic_missile: called without a creatureManager");
	assert(ctx.creatures && "SpellSystem::cast_magic_missile: called without creatures");
	assert(ctx.map && "SpellSystem::cast_magic_missile: called without a map");
	assert(ctx.dice && "SpellSystem::cast_magic_missile: called without dice");

	// Find all valid targets in FOV
	std::vector<Creature*> targets;
	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "cast_magic_missile: creatures list holds a null entry");

		if (!creature->is_dead())
		{
			if (ctx.map->is_in_fov(creature->position))
			{
				targets.push_back(creature.get());
			}
		}
	}

	if (targets.empty())
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "No valid target in sight!", MessageCompletion::FINISHED);
		return false;
	}

	resolve_magic_missile(caster, std::move(targets), ctx);
	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
	return true;
}

void resolve_magic_missile(Creature& caster, std::vector<Creature*> targets, GameContext& ctx)
{
	assert(ctx.dice && "SpellSystem::resolve_magic_missile: called without dice");
	assert(ctx.buffSystem && "SpellSystem::resolve_magic_missile: called without a buffSystem");

	const int numMissiles = calculate_num_missiles(caster_level(caster));

	// Sort by distance (nearest first)
	std::sort(targets.begin(), targets.end(), [&caster](Creature* left, Creature* right)
		{ return caster.get_tile_distance(left->position) < caster.get_tile_distance(right->position); });

	int totalDamage = 0;
	std::unordered_map<Creature*, int> damagePerTarget;

	// Fire missiles - distribute among targets, prioritizing nearest
	for (int missileIndex = 0; missileIndex < numMissiles; ++missileIndex)
	{
		// Target nearest living enemy the caster may strike; one whose Sanctuary turns
		// the caster away is ignored, and the missile goes to the next.
		Creature* target = nullptr;
		for (Creature* candidate : targets)
		{
			if (!candidate->is_dead() && !ctx.buffSystem->is_turned_away_by_sanctuary(caster, *candidate, ctx))
			{
				target = candidate;
				break;
			}
		}

		if (!target)
		{
			break;
		}

		SpellAnimations::animate_magic_missile(caster.position, target->position, ctx);

		int damage = ctx.dice->roll(1, 4) + 1;
		totalDamage += damage;
		damagePerTarget[target] += damage;

		target->take_damage_and_check_death(damage, ctx, DamageType::MAGIC);
	}

	// Message
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, std::format("Magic Missile ({})! ", numMissiles));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "Total ");
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, std::format("{} damage!", totalDamage));
	ctx.messageSystem->finalize_message();
}

bool cast_shield(Creature& caster, GameContext& ctx)
{
	// A flat 5 rounds, where the book (PHB page 278) gives 5 rounds per level.
	const int duration = 5;

	// Spell: ADD +4 AC
	ctx.buffSystem->add_buff(caster, BuffType::SHIELD, 4, duration, BuffApplication::ADD, ctx.gameState->get_time());
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Shield! ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format("+4 AC for {}.", Wording::rounds(duration)));
	ctx.messageSystem->finalize_message();
	return true;
}

bool cast_sleep(Creature& caster, GameContext& ctx)
{
	assert(ctx.creatures && "SpellSystem::cast_sleep: called without creatures");
	assert(ctx.map && "SpellSystem::cast_sleep: called without a map");
	assert(ctx.dice && "SpellSystem::cast_sleep: called without dice");

	// PHB p.279: 2d4 Hit Dice of monsters, fewest first, 5 rounds a level; undead and 4+3 HD are immune.
	// Creatures here carry whole Hit Dice, so 4+3 becomes anything above four.
	constexpr int SLEEP_MAX_HIT_DICE = 4;
	constexpr int SLEEP_ROUNDS_PER_LEVEL = 5;

	int hdBudget = Spells::roll_sleep_hit_dice_budget(*ctx.dice);
	const int duration = SLEEP_ROUNDS_PER_LEVEL * caster_level(caster);

	// Gather the eligible first: the budget is spent weakest-first, which the
	// container's own order does not give.
	std::vector<Creature*> sleepable{};
	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "cast_sleep: creatures list holds a null entry");

		if (creature->is_dead() || creature->is_undead())
		{
			continue;
		}

		if (creature->get_hit_dice() > SLEEP_MAX_HIT_DICE)
		{
			continue;
		}

		if (!ctx.map->is_in_fov(creature->position))
		{
			continue;
		}

		sleepable.push_back(creature.get());
	}

	std::ranges::sort(sleepable,
		[](const Creature* left, const Creature* right)
		{ return left->get_hit_dice() < right->get_hit_dice(); });

	int affected = 0;
	for (Creature* creature : sleepable)
	{
		const int hitDice = creature->get_hit_dice();

		// Partial effects are ignored: a creature the budget cannot cover whole
		// is left awake, and the budget stops there.
		if (hitDice > hdBudget)
		{
			break;
		}

		ctx.buffSystem->add_buff(*creature, BuffType::SLEEP, 0, duration, BuffApplication::ADD, ctx.gameState->get_time());
		hdBudget -= hitDice;
		++affected;
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Sleep! ");
		ctx.messageSystem->append_message_part(
			ColorPairId::WHITE_BLACK, std::format("{} put to sleep.", Wording::counted(affected, "creature", "creatures")));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Sleep spell has no effect.", MessageCompletion::FINISHED);
	}

	return true;
}

bool cast_hold_person(Creature& caster, GameContext& ctx)
{
	assert(ctx.creatures && "SpellSystem::cast_hold_person: called without creatures");
	assert(ctx.map && "SpellSystem::cast_hold_person: called without a map");
	assert(ctx.dice && "SpellSystem::cast_hold_person: called without dice");

	// AD&D 2e: paralyzes up to 1d4 humanoids in FOV; save vs. spells (d20 >= 15) negates
	// Duration: 2 rounds per caster level
	int casterLevel = caster_level(caster);
	int duration = 2 * casterLevel;
	int maxTargets = ctx.dice->roll(1, 4);
	int affected = 0;

	for (const auto& creature : *ctx.creatures)
	{
		if (affected >= maxTargets)
		{
			break;
		}
		assert(creature && "cast_hold_person: creatures list holds a null entry");

		if (creature->is_dead())
		{
			continue;
		}
		if (!ctx.map->is_in_fov(creature->position))
		{
			continue;
		}

		// The caster selects each person; one whose Sanctuary turns the caster away is
		// not selected, and the hold goes to another (PHB page 309).
		if (ctx.buffSystem->is_turned_away_by_sanctuary(caster, *creature, ctx))
		{
			continue;
		}

		if (!SavingThrows::is_made(*creature, SavingThrow::SPELL, 0, ctx))
		{
			ctx.buffSystem->add_buff(*creature, BuffType::HOLD_PERSON, 0, duration, BuffApplication::ADD, ctx.gameState->get_time());
			++affected;
		}
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Hold Person! ");
		ctx.messageSystem->append_message_part(
			ColorPairId::WHITE_BLACK, std::format("{} paralyzed for {}.", Wording::counted(affected, "creature", "creatures"), Wording::rounds(duration)));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Hold Person has no effect.", MessageCompletion::FINISHED);
	}

	return true;
}

// PHB page 291: the spell lasts "until the recipient attacks any creature, or until 24
// hours have passed". The attack break is BuffSystem's; this is the day.
inline constexpr int INVISIBILITY_SPELL_ROUNDS = 24 * ROUNDS_PER_HOUR;

bool cast_invisibility(Creature& caster, BuffType granted, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, granted, 0, INVISIBILITY_SPELL_ROUNDS, BuffApplication::ADD, ctx.gameState->get_time());
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Invisibility! ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You fade from view.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool cast_teleport(Creature& caster, GameContext& ctx)
{
	assert(ctx.map && "SpellSystem::cast_teleport: called without a map");

	// The casting is spent with nowhere to go, as Knock's is with no door in reach.
	const std::optional<Vector2D> landing = SpawnUtils::find_random_floor_tile(ctx);
	if (!landing)
	{
		ctx.messageSystem->append_message_part(ColorPairId::MAGENTA_BLACK, "Teleport! ");
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "The magic finds nowhere to take you.");
		ctx.messageSystem->finalize_message();
		return true;
	}

	caster.position = *landing;
	ctx.map->compute_fov(ctx);

	ctx.messageSystem->append_message_part(ColorPairId::MAGENTA_BLACK, "Teleport! ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You feel disoriented as the world shifts around you!");
	ctx.messageSystem->finalize_message();
	return true;
}

bool cast_knock(Creature& caster, GameContext& ctx)
{
	assert(ctx.map && "SpellSystem::cast_knock: called without a map");

	// AD&D 2e: Knock opens any locked door within range.
	// Scan a 7-tile radius for the nearest locked door and open it.
	constexpr int KNOCK_RADIUS = 7;

	Vector2D nearest{ -1, -1 };
	int bestDist = KNOCK_RADIUS + 1;

	for (int deltaRow = -KNOCK_RADIUS; deltaRow <= KNOCK_RADIUS; ++deltaRow)
	{
		for (int deltaCol = -KNOCK_RADIUS; deltaCol <= KNOCK_RADIUS; ++deltaCol)
		{
			Vector2D candidate{
				caster.position.x + deltaCol,
				caster.position.y + deltaRow,
			};
			if (!ctx.map->is_in_bounds(candidate))
			{
				continue;
			}
			if (ctx.map->is_door_locked(candidate))
			{
				int dist = std::abs(deltaCol) + std::abs(deltaRow);
				if (dist < bestDist)
				{
					bestDist = dist;
					nearest = candidate;
				}
			}
		}
	}

	if (nearest.x < 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Knock! ");
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "No locked doors are nearby.");
		ctx.messageSystem->finalize_message();
		return true; // Spell slot consumed even on miss - AD&D 2e rule
	}

	ctx.map->unlock_door(nearest);
	ctx.map->open_door(nearest, ctx);
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Knock! ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "The lock clicks open.");
	ctx.messageSystem->finalize_message();
	return true;
}

} // namespace
} // namespace SpellSystem

std::vector<int> SpellSystem::progression_slots(CasterClass classState, int level)
{
	// AD&D 2e spell progression tables
	// Returns slots per spell level [level1, level2, level3, ...]

	// Priest Spell Progression, PHB Table 24 (PDF page 72), levels 1 to 20. Columns six and
	// seven need Wisdom 17 and 18, which get_spell_slots applies.
	if (classState == CasterClass::CLERIC)
	{
		static const std::vector<std::vector<int>> clericSlots = {
			{ 1 },
			{ 2 },
			{ 2, 1 },
			{ 3, 2 },
			{ 3, 3, 1 },
			{ 3, 3, 2 },
			{ 3, 3, 2, 1 },
			{ 3, 3, 3, 2 },
			{ 4, 4, 3, 2, 1 },
			{ 4, 4, 3, 3, 2 },
			{ 5, 4, 4, 3, 2, 1 },
			{ 6, 5, 5, 3, 2, 2 },
			{ 6, 6, 6, 4, 2, 2 },
			{ 6, 6, 6, 5, 3, 2, 1 },
			{ 6, 6, 6, 6, 4, 2, 1 },
			{ 7, 7, 7, 6, 4, 3, 1 },
			{ 7, 7, 7, 7, 5, 3, 2 },
			{ 8, 8, 8, 8, 6, 4, 2 },
			{ 9, 9, 8, 8, 6, 4, 2 },
			{ 9, 9, 9, 8, 7, 5, 2 },
		};
		return row_for_level(clericSlots, level);
	}

	// Wizard Spell Progression, Table 21 (PDF page 66), to 20th level.
	if (classState == CasterClass::WIZARD)
	{
		static const std::vector<std::vector<int>> wizardSlots = {
			{ 1 },
			{ 2 },
			{ 2, 1 },
			{ 3, 2 },
			{ 4, 2, 1 },
			{ 4, 2, 2 },
			{ 4, 3, 2, 1 },
			{ 4, 3, 3, 2 },
			{ 4, 3, 3, 2, 1 },
			{ 4, 4, 3, 2, 2 },
			{ 4, 4, 4, 3, 3 },
			{ 4, 4, 4, 4, 4, 1 },
			{ 5, 5, 5, 4, 4, 2 },
			{ 5, 5, 5, 4, 4, 2, 1 },
			{ 5, 5, 5, 5, 5, 2, 1 },
			{ 5, 5, 5, 5, 5, 3, 2, 1 },
			{ 5, 5, 5, 5, 5, 3, 3, 2 },
			{ 5, 5, 5, 5, 5, 3, 3, 2, 1 },
			{ 5, 5, 5, 5, 5, 3, 3, 3, 1 },
			{ 5, 5, 5, 5, 5, 4, 3, 3, 2 },
		};
		return row_for_level(wizardSlots, level);
	}

	return {};
}

std::vector<int> SpellSystem::bonus_priest_spells(int wisdom, const DataManager& dataManager)
{
	// "Bonus spells are cumulative", so a score is worth every row up to it.
	std::vector<int> perSpellLevel;
	for (const WisdomAttributes& row : dataManager.get_wisdom_attributes())
	{
		if (row.Wis > wisdom)
		{
			break;
		}

		for (const int spellLevel : row.bonusSpells)
		{
			if (static_cast<std::size_t>(spellLevel) > perSpellLevel.size())
			{
				perSpellLevel.resize(static_cast<std::size_t>(spellLevel), 0);
			}
			++perSpellLevel.at(static_cast<std::size_t>(spellLevel) - 1);
		}
	}
	return perSpellLevel;
}

std::vector<int> SpellSystem::get_spell_slots(CasterClass classState, int level, int wisdom, const DataManager& dataManager)
{
	std::vector<int> slots = progression_slots(classState, level);

	// Table 5's bonus spells and Table 24's two footnotes are a priest's alone.
	if (classState != CasterClass::CLERIC)
	{
		return slots;
	}

	// "* Usable only by priests with 17 or greater Wisdom" on the sixth column, and
	// 18 on the seventh, so a priest below those never reaches those rows.
	std::size_t reachableRows = SIXTH_SPELL_LEVEL_ROW;
	if (wisdom >= WISDOM_FOR_SIXTH_LEVEL)
	{
		reachableRows = SEVENTH_SPELL_LEVEL_ROW;
	}
	if (wisdom >= WISDOM_FOR_SEVENTH_LEVEL)
	{
		reachableRows = SEVENTH_SPELL_LEVEL_ROW + 1;
	}
	if (slots.size() > reachableRows)
	{
		slots.resize(reachableRows);
	}

	// "these spells are available only when the priest is entitled to spells of the
	// appropriate level", so a bonus with no row of its own is not granted.
	const std::vector<int> bonus = bonus_priest_spells(wisdom, dataManager);
	for (std::size_t index = 0; index < bonus.size() && index < slots.size(); ++index)
	{
		slots.at(index) += bonus.at(index);
	}
	return slots;
}

int SpellSystem::highest_spell_level(CasterClass classState, int level, int wisdom, const DataManager& dataManager)
{
	return static_cast<int>(get_spell_slots(classState, level, wisdom, dataManager).size());
}

int SpellSystem::spell_failure_chance(const Creature& caster, const DataManager& dataManager)
{
	// Table 5 is Wisdom's, and only priests risk a fizzle; a wizard casts on Intelligence.
	if (caster_class(caster) != CasterClass::CLERIC)
	{
		return 0;
	}
	return dataManager.wisdom_for(caster.get_wisdom()).ChanceOfSpellFailure;
}

bool SpellSystem::spell_fizzles(const Creature& caster, const DataManager& dataManager, RandomDice& dice)
{
	// A caster the table gives no chance is not made to roll for one.
	const int chance = spell_failure_chance(caster, dataManager);
	if (chance <= 0)
	{
		return false;
	}

	// "if the number rolled is less than or equal to the listed chance for spell
	// failure, the spell is expended with absolutely no effect whatsoever."
	return dice.d100() <= chance;
}

namespace SpellSystem
{
namespace
{
void cast_with_targets(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	const SpellTargets& targets,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx)
{
	assert(ctx.dataManager && "SpellSystem::cast_spell_by_key: called without a dataManager");
	assert(ctx.spellRegistry && "SpellSystem::cast_spell_by_key: called without a spellRegistry");
	assert(ctx.dice && "SpellSystem::cast_spell_by_key: called without dice");

	// Every successful effect calls this, sometimes a turn later; an empty one would throw far from its cause.
	assert(onCastComplete && "cast_spell_by_key requires a callback");

	if (caster.has_state(ActorState::IS_SILENCED))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You are silenced and cannot cast spells!", MessageCompletion::FINISHED);
		return;
	}
	// A priest's own spell can fail on its Wisdom. The casting is over either way -
	// the spell is expended - so the callback runs and only the effect is skipped.
	if (source == SpellSource::MEMORIZED && spell_fizzles(caster, *ctx.dataManager, *ctx.dice))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your spell fizzles.", MessageCompletion::FINISHED);
		onCastComplete(ctx);
		return;
	}

	const SpellDefinition& definition = ctx.spellRegistry->get_by_key(key);
	dispatch_effect(definition.effect_type, caster, source, targets, std::move(onCastComplete), ctx);
}
} // namespace
} // namespace SpellSystem

void SpellSystem::cast_spell_by_key(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx)
{
	cast_with_targets(key, caster, source, PlayerChooses{}, std::move(onCastComplete), ctx);
}

SpellSystem::FireballBurst SpellSystem::burst_fireball(Vector2D center, int casterLevel, int radius, GameContext& ctx)
{
	assert(ctx.creatures && "SpellSystem::burst_fireball: called without creatures");
	assert(ctx.dice && "SpellSystem::burst_fireball: called without dice");

	// AD&D 2e: 1d6 per caster level, max 10d6
	FireballBurst burst{};
	burst.diceCount = std::min(casterLevel, 10);
	for (int die = 0; die < burst.diceCount; ++die)
	{
		burst.dice.push_back(ctx.dice->roll(1, 6));
		burst.totalDamage += burst.dice.back();
	}

	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "burst_fireball: creatures list holds a null entry");

		if (creature->is_dead())
		{
			continue;
		}
		if (creature->get_tile_distance(center) > static_cast<double>(radius))
		{
			continue;
		}

		burn_with_fireball(*creature, burst, ctx);
		++burst.struck;
	}

	// The player is never in ctx.creatures, and the burst reaches the reader or caster all the same.
	Creature* player = ctx.player();
	assert(player && "burst_fireball: called without a player");
	if (!player->is_dead() && player->get_tile_distance(center) <= static_cast<double>(radius))
	{
		burn_with_fireball(*player, burst, ctx);
		++burst.struck;
		burst.playerReach = PlayerReach::STRUCK;
	}

	return burst;
}

void SpellSystem::burn_with_fireball(Creature& target, const FireballBurst& burst, GameContext& ctx)
{
	const int strength = DamageResolver::resistance_strength(DamageType::FIRE, target, ctx);

	// AD&D 2e: a save versus spell halves the damage, with the ring's bonus
	// added to the roll as the book adds it.
	const bool saved = SavingThrows::is_made(
		target,
		SavingThrow::SPELL,
		DamageResolver::save_bonus_against(DamageType::FIRE, target, ctx),
		ctx);
	const DamageResolver::ResistedDamage reduced = DamageResolver::reduce_dice(burst.dice, DamageType::FIRE, strength);
	const DamageResolver::ResistedDamage dealt = saved ? reduced.at(reduced.hit_points() / 2) : reduced;
	target.take_damage_and_check_death(dealt, ctx);
}

void SpellSystem::cast_spell_at(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	Creature& target,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx)
{
	assert(ctx.spellRegistry && "SpellSystem::cast_spell_at: called without a spellRegistry");
	assert(can_aim_at(ctx.spellRegistry->get_by_key(key).effect_type, caster, target, ctx) && "SpellSystem::cast_spell_at: a spell was aimed where it cannot land");

	cast_with_targets(key, caster, source, AimedAt{ target }, std::move(onCastComplete), ctx);
}

bool SpellSystem::can_aim_at(SpellEffectType effect, const Creature& caster, const Creature& target, GameContext& ctx)
{
	assert(ctx.map && "SpellSystem::can_aim_at: called without a map");

	// Every spell that can be aimed needs its target alive and seen.
	if (target.is_dead() || !ctx.map->has_los(caster.position, target.position))
	{
		return false;
	}
	if (effect == SpellEffectType::MAGIC_MISSILE)
	{
		return true;
	}
	if (effect == SpellEffectType::FIREBALL)
	{
		const int distance = caster.get_tile_distance(target.position);
		return distance <= fireball_range(caster_level(caster)) && distance > FIREBALL_RADIUS;
	}
	return false;
}

CasterClass SpellSystem::caster_class(const Creature& caster)
{
	if (caster.spellcasting.training)
	{
		return caster.spellcasting.training->casterClass;
	}
	return to_caster_class(caster.get_creature_class());
}

int SpellSystem::caster_level(const Creature& caster)
{
	if (caster.spellcasting.training)
	{
		return caster.spellcasting.training->casterLevel;
	}
	return caster.get_creature_level();
}

std::vector<std::string> SpellSystem::spells_to_memorize(
	CasterClass casterClass,
	int casterLevel,
	int wisdom,
	const SpellRegistry& spells,
	const DataManager& dataManager)
{
	const std::vector<int> slots = get_spell_slots(casterClass, casterLevel, wisdom, dataManager);
	const int maxSpellLevel = static_cast<int>(slots.size());
	const std::vector<std::string> available = spells.get_available_spells(casterClass, maxSpellLevel);

	std::vector<std::string> memorized;
	for (int level = 1; level <= maxSpellLevel; ++level)
	{
		int slotsAtLevel = slots.at(static_cast<size_t>(level) - 1);
		for (const std::string& key : available)
		{
			if (spells.get_by_key(key).level == level && slotsAtLevel > 0)
			{
				memorized.push_back(key);
				--slotsAtLevel;
			}
		}
	}
	return memorized;
}

void SpellSystem::memorize_spells(Creature& caster, const SpellRegistry& spells, const DataManager& dataManager)
{
	caster.spellcasting.memorizedSpells = spells_to_memorize(caster_class(caster), caster_level(caster), caster.get_wisdom(), spells, dataManager);
}

void SpellSystem::show_memorization_menu(Player& player, GameContext& ctx)
{
	assert(ctx.dataManager && "SpellSystem::show_memorization_menu: called without a dataManager");
	assert(ctx.spellRegistry && "SpellSystem::show_memorization_menu: called without a spellRegistry");

	const std::vector<int> slots = get_spell_slots(caster_class(player), caster_level(player), player.get_wisdom(), *ctx.dataManager);
	if (slots.empty())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You cannot cast spells.", MessageCompletion::FINISHED);
		return;
	}

	memorize_spells(player, *ctx.spellRegistry, *ctx.dataManager);

	const std::vector<std::string>& memorized = player.spellcasting.memorizedSpells;
	ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, "Spells memorized: ");
	for (size_t spellIndex = 0; spellIndex < memorized.size(); ++spellIndex)
	{
		if (spellIndex > 0)
		{
			ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ", ");
		}
		ctx.messageSystem->append_message_part(ColorPairId::GREEN_BLACK, ctx.spellRegistry->get_by_key(memorized[spellIndex]).name);
	}
	ctx.messageSystem->finalize_message();
}

void SpellSystem::show_casting_menu(Player& player, GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuSpellCast>(player, ctx));
}

std::vector<SpellSystem::ItemGrantedSpell> SpellSystem::get_item_granted_spells(const Player& player, int now)
{
	std::vector<ItemGrantedSpell> itemSpells;

	// Check for Ring of Invisibility, once however many are worn
	if (player.wears_ring_of(MagicalEffect::INVISIBILITY))
	{
		itemSpells.push_back({ "invisibility", "Ring", ItemPower::CAST_SPELL });
	}

	// Check for Helm of Teleportation
	if (Item* helm = player.get_equipped_item(EquipmentSlot::HEAD))
	{
		if (is_helm_ready(*helm, now))
		{
			itemSpells.push_back({ "teleport", "Helm", ItemPower::CAST_SPELL_ONCE_A_DAY });
		}

		// The helm of brilliance's word, once identifying the helm has taught it.
		if (is_awaiting_command_word(*helm))
		{
			itemSpells.push_back({ "", "Helm", ItemPower::SPEAK_COMMAND_WORD });
		}
	}

	return itemSpells;
}
