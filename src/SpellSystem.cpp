#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Actor.h"
#include "EquipmentSlot.h"
#include "Pickable.h"
#include "Player.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "DamageResolver.h"
#include "GameContext.h"
#include "MagicalItemEffects.h"
#include "Map.h"
#include "MenuSpellCast.h"
#include "Vector2D.h"
#include "SavingThrow.h"
#include "AnimationSystem.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "CreatureManager.h"
#include "MessageSystem.h"
#include "SpawnUtils.h"
#include "SpellAnimations.h"
#include "SpellRegistry.h"
#include "SpellSystem.h"
#include "TargetingMenu.h"
#include "TileConfig.h"

namespace
{
// Helper to convert PlayerClassState to CasterClass
CasterClass to_caster_class(Player::PlayerClassState state)
{
	switch (state)
	{

	case Player::PlayerClassState::CLERIC:
	{
		return CasterClass::CLERIC;
	}

	case Player::PlayerClassState::WIZARD:
	{
		return CasterClass::WIZARD;
	}

	default:
	{
		return CasterClass::NONE;
	}

	}
}
} // namespace

std::vector<int> SpellSystem::get_spell_slots(CasterClass classState, int level)
{
	// AD&D 2e spell progression tables
	// Returns slots per spell level [level1, level2, level3, ...]

	if (classState == CasterClass::CLERIC)
	{
		// Cleric spell progression
		static const std::vector<std::vector<int>> clericSlots = {
			{ 1 }, // Level 1
			{ 2 }, // Level 2
			{ 2, 1 }, // Level 3
			{ 3, 2 }, // Level 4
			{ 3, 3, 1 }, // Level 5
			{ 3, 3, 2 }, // Level 6
			{ 3, 3, 2, 1 }, // Level 7
			{ 3, 3, 3, 2 }, // Level 8
			{ 4, 4, 3, 2, 1 }, // Level 9
			{ 4, 4, 3, 3, 2 }, // Level 10
		};
		int idx = std::min(level, 10) - 1;
		return idx >= 0 ? clericSlots[idx] : std::vector<int>{};
	}
	else if (classState == CasterClass::WIZARD)
	{
		// Wizard spell progression
		static const std::vector<std::vector<int>> wizardSlots = {
			{ 1 }, // Level 1
			{ 2 }, // Level 2
			{ 2, 1 }, // Level 3
			{ 3, 2 }, // Level 4
			{ 4, 2, 1 }, // Level 5
			{ 4, 2, 2 }, // Level 6
			{ 4, 3, 2, 1 }, // Level 7
			{ 4, 3, 3, 2 }, // Level 8
			{ 4, 3, 3, 2, 1 }, // Level 9
			{ 4, 4, 3, 2, 2 }, // Level 10
		};
		int idx = std::min(level, 10) - 1;
		return idx >= 0 ? wizardSlots[idx] : std::vector<int>{};
	}

	return {};
}

void SpellSystem::dispatch_effect(
	SpellEffectType effect,
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// Targeted spells: async via TargetingMenu — onSuccess is forwarded into the callback
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
		break;

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
		result = cast_invisibility(caster, ctx);
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
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Spell not implemented yet.", true);
		break;
	}

	}

	if (result)
	{
		onSuccess(ctx);
	}
}

void SpellSystem::cast_spell_by_key(
	std::string_view key,
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// Every effect that succeeds calls this, at four sites and sometimes a turn
	// later from a targeting callback. An empty one throws there rather than
	// here, which is a long way from the caller that omitted it.
	assert(onSuccess && "cast_spell_by_key requires a callback");

	if (caster.has_state(ActorState::IS_SILENCED))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are silenced and cannot cast spells!", true);
		return;
	}
	const SpellDefinition& definition = ctx.spellRegistry->get_by_key(key);
	dispatch_effect(definition.effect_type, caster, std::move(onSuccess), ctx);
}

namespace
{
void animate_heal(const Vector2D& pos, GameContext& ctx)
{
	if (!ctx.animSystem)
	{
		return;
	}

	ctx.animSystem->spawn_effect(
		pos,
		ctx.tileConfig->get("TILE_EFFECT_HEAL"),
		60,
		220,
		60,
		0.5f);
}
} // namespace

bool SpellSystem::cast_cure_light_wounds(Creature& caster, GameContext& ctx)
{
	animate_heal(caster.position, ctx);

	// Magical healing, which reaches the fire and acid wounds regeneration cannot.
	const int actualHealing = caster.heal(ctx.dice->roll(1, 8));

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Cure Light Wounds! ");
	ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP", actualHealing));
	ctx.messageSystem->finalize_message();

	return true;
}

bool SpellSystem::cast_bless(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::BLESS, 0, 6, false); // Spell: ADD effect
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Bless! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "+1 to hit for 6 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

// AD&D 2e, Player's Handbook page 277: evil creatures attacking the protected
// suffer -2 on attack rolls. Duration is 2 rounds per caster level.
//
// Only the to-hit penalty is implemented. The spell's other two effects -
// blocking mental control, and hedging out conjured creatures - have nothing
// in this game to act on yet.
bool SpellSystem::cast_protection_from_evil(Creature& caster, GameContext& ctx)
{
	const int duration = 2 * caster.get_creature_level();

	ctx.buffSystem->add_buff(caster, BuffType::PROTECTION_FROM_EVIL, PROTECTION_FROM_EVIL_PENALTY, duration, false);

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Protection From Evil! ");
	ctx.messageSystem->append_message_part(
		WHITE_BLACK_PAIR,
		std::format("Evil creatures strike at {} against you for {} turns.",
			PROTECTION_FROM_EVIL_PENALTY, duration));
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_sanctuary(Creature& caster, GameContext& ctx)
{
	// AD&D 2e, PHB page 436: 2 rounds + 1 round/level. Cancelled by attacking.
	const int casterLevel = caster.get_creature_level();
	const int duration = 2 + casterLevel;

	ctx.buffSystem->add_buff(caster, BuffType::SANCTUARY, 0, duration, false);

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Sanctuary! ");
	ctx.messageSystem->append_message_part(
		WHITE_BLACK_PAIR,
		std::format("Divine protection shields you for {} turns.", duration));
	ctx.messageSystem->finalize_message();
	return true;
}

void SpellSystem::cast_silence(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Duration 2 rounds/level. Targeted single creature. Prevents spellcasting.
	int casterLevel = caster.get_creature_level();
	int duration = 2 * casterLevel;
	int range = 5 + casterLevel;

	auto onTarget = [duration, onSuccess = std::move(onSuccess)](
		bool confirmed,
		Vector2D targetPos,
		GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Silence cancelled.", true);
			return;
		}

		Creature* target = nullptr;
		for (const auto& creature : *innerCtx.creatures)
		{
			if (creature && creature->position == targetPos && !creature->is_dead())
			{
				target = creature.get();
				break;
			}
		}

		if (!target)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "No creature at that location.", true);
			return;
		}

		innerCtx.buffSystem->add_buff(*target, BuffType::SILENCE, 0, duration, false);
		SpellAnimations::animate_creature_hit(target->position, innerCtx);

		innerCtx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Silence! ");
		innerCtx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR,
			std::format("{} is struck mute for {} turns.", target->get_name(), duration));
		innerCtx.messageSystem->finalize_message();

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, 0, std::move(onTarget), ctx));
}

void SpellSystem::cast_web(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Save vs. Paralyzation (d20 >= 10) or be entangled. Duration 2 rounds/level.
	int casterLevel = caster.get_creature_level();
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
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Web cancelled.", true);
			return;
		}

		int affected = 0;
		for (const auto& creature : *innerCtx.creatures)
		{
			if (!creature || creature->is_dead())
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
				innerCtx.buffSystem->add_buff(*creature, BuffType::WEBBED, 0, duration, false);
				SpellAnimations::animate_creature_hit(creature->position, innerCtx);
				++affected;
			}
		}

		if (affected > 0)
		{
			innerCtx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Web! ");
			innerCtx.messageSystem->append_message_part(
				WHITE_BLACK_PAIR,
				std::format(
					"{} creature{} entangled for {} turns.",
					affected,
					affected == 1 ? " is" : "s are",
					duration));
			innerCtx.messageSystem->finalize_message();
		}
		else
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "The webs spread but catch nothing.", true);
		}

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, radius, std::move(onTarget), ctx));
}

SpellSystem::FireballBurst SpellSystem::burst_fireball(Vector2D center, int casterLevel, int radius, GameContext& ctx)
{
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
		if (!creature || creature->is_dead())
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

void SpellSystem::cast_fireball(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	int casterLevel = caster.get_creature_level();

	// AD&D 2e: range 10" + 1"/level (tiles), AoE 20-ft radius (2 tiles)
	int range = 10 + casterLevel;
	int radius = 2;

	auto onTarget = [casterLevel, radius, onSuccess = std::move(onSuccess)](
		bool confirmed,
		Vector2D center,
		GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Fireball cancelled.", true);
			return;
		}

		SpellAnimations::animate_explosion(center, radius, innerCtx);

		const FireballBurst burst = burst_fireball(center, casterLevel, radius, innerCtx);

		innerCtx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Fireball! ");
		innerCtx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{}d6 = {} damage", burst.diceCount, burst.totalDamage));
		if (burst.struck > 0)
		{
			innerCtx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({} struck)", burst.struck));
		}
		innerCtx.messageSystem->finalize_message();

		innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, radius, std::move(onTarget), ctx));
}

namespace
{
// AD&D 2e: 1 missile at level 1, +1 every 2 levels, max 5
int calculate_num_missiles(int casterLevel)
{
	return std::min(5, 1 + (casterLevel - 1) / 2);
}
} // namespace

bool SpellSystem::cast_magic_missile(Creature& caster, GameContext& ctx)
{
	// Get caster level
	int casterLevel = caster.get_creature_level();
	int numMissiles = calculate_num_missiles(casterLevel);

	// Find all valid targets in FOV
	std::vector<Creature*> targets;
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && !creature->is_dead())
		{
			if (ctx.map->is_in_fov(creature->position))
			{
				targets.push_back(creature.get());
			}
		}
	}

	if (targets.empty())
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, "No valid target in sight!", true);
		return false;
	}

	// Sort by distance (nearest first)
	std::sort(targets.begin(), targets.end(), [&caster](Creature* left, Creature* right)
		{ return caster.get_tile_distance(left->position) < caster.get_tile_distance(right->position); });

	int totalDamage = 0;
	std::unordered_map<Creature*, int> damagePerTarget;

	// Fire missiles - distribute among targets, prioritizing nearest
	for (int i = 0; i < numMissiles; ++i)
	{
		// Target nearest living enemy the caster may strike; one whose Sanctuary turns
		// the caster away is ignored, and the missile goes to the next.
		Creature* target = nullptr;
		for (Creature* t : targets)
		{
			if (!t->is_dead() && !ctx.buffSystem->is_turned_away_by_sanctuary(caster, *t, ctx))
			{
				target = t;
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
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, std::format("Magic Missile ({})! ", numMissiles));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Total ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{} damage!", totalDamage));
	ctx.messageSystem->finalize_message();

	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);

	return true;
}

bool SpellSystem::cast_shield(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::SHIELD, 4, 5, false); // Spell: ADD +4 AC
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Shield! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "+4 AC for 5 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_sleep(Creature& caster, GameContext& ctx)
{
	// AD&D 2e, Player's Handbook page 279: the spell affects 2d4 Hit Dice of
	// monsters, least Hit Dice first, and lasts 5 rounds per caster level.
	// Undead and creatures of 4+3 Hit Dice or more are unaffected.
	//
	// The book's threshold is four Hit Dice plus three hit points. Creatures
	// here carry whole Hit Dice, so anything above four is out of reach.
	constexpr int SLEEP_MAX_HIT_DICE = 4;
	constexpr int SLEEP_ROUNDS_PER_LEVEL = 5;

	int hdBudget = ctx.dice->roll(2, 4);
	const int duration = SLEEP_ROUNDS_PER_LEVEL * caster.get_creature_level();

	// Gather the eligible first: the budget is spent weakest-first, which the
	// container's own order does not give.
	std::vector<Creature*> sleepable{};
	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "creatures list holds a null entry");

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

		ctx.buffSystem->add_buff(*creature, BuffType::SLEEP, 0, duration, false);
		hdBudget -= hitDice;
		++affected;
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Sleep! ");
		ctx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR, std::format("{} creatures fall asleep.", affected));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Sleep spell has no effect.", true);
	}

	return true;
}

bool SpellSystem::cast_hold_person(Creature& caster, GameContext& ctx)
{
	// AD&D 2e: paralyzes up to 1d4 humanoids in FOV; save vs. spells (d20 >= 15) negates
	// Duration: 2 rounds per caster level
	int casterLevel = caster.get_creature_level();
	int duration = 2 * casterLevel;
	int maxTargets = ctx.dice->roll(1, 4);
	int affected = 0;

	for (const auto& creature : *ctx.creatures)
	{
		if (affected >= maxTargets)
		{
			break;
		}
		if (!creature || creature->is_dead())
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
			ctx.buffSystem->add_buff(*creature, BuffType::HOLD_PERSON, 0, duration, false);
			++affected;
		}
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Hold Person! ");
		ctx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR, std::format("{} creatures paralyzed for {} turns.", affected, duration));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Hold Person has no effect.", true);
	}

	return true;
}

bool SpellSystem::cast_invisibility(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::INVISIBILITY, 0, 20, false); // Spell: ADD effect
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Invisibility! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You fade from view for 20 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_teleport(Creature& caster, GameContext& ctx)
{
	caster.position = SpawnUtils::find_random_floor_tile(ctx);
	ctx.map->compute_fov(ctx);

	ctx.messageSystem->append_message_part(MAGENTA_BLACK_PAIR, "Teleport! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You feel disoriented as the world shifts around you!");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_knock(Creature& caster, GameContext& ctx)
{
	// AD&D 2e: Knock opens any locked door within range.
	// Scan a 7-tile radius for the nearest locked door and open it.
	constexpr int KNOCK_RADIUS = 7;

	Vector2D nearest{ -1, -1 };
	int bestDist = KNOCK_RADIUS + 1;

	for (int dy = -KNOCK_RADIUS; dy <= KNOCK_RADIUS; ++dy)
	{
		for (int dx = -KNOCK_RADIUS; dx <= KNOCK_RADIUS; ++dx)
		{
			Vector2D candidate{ caster.position.x + dx, caster.position.y + dy };
			if (!ctx.map->is_in_bounds(candidate))
			{
				continue;
			}
			if (ctx.map->is_door_locked(candidate))
			{
				int dist = std::abs(dx) + std::abs(dy);
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
		ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Knock! ");
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "No locked doors are nearby.");
		ctx.messageSystem->finalize_message();
		return true; // Spell slot consumed even on miss — AD&D 2e rule
	}

	ctx.map->unlock_door(nearest);
	ctx.map->open_door(nearest, ctx);
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Knock! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "The lock clicks open.");
	ctx.messageSystem->finalize_message();
	return true;
}

void SpellSystem::show_memorization_menu(Player& player, GameContext& ctx)
{
	CasterClass casterClass = to_caster_class(player.playerClassState);
	auto slots = get_spell_slots(casterClass, player.get_creature_level());
	if (slots.empty())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You cannot cast spells.", true);
		return;
	}

	int maxSpellLevel = static_cast<int>(slots.size());
	auto available = ctx.spellRegistry->get_available_spells(casterClass, maxSpellLevel);

	// Clear current memorized spells
	player.memorizedSpells.clear();

	// Auto-memorize spells to fill slots (simplified)
	for (int level = 1; level <= maxSpellLevel; ++level)
	{
		int slotsAtLevel = slots[level - 1];
		for (const std::string& key : available)
		{
			const SpellDefinition& definition = ctx.spellRegistry->get_by_key(key);
			if (definition.level == level && slotsAtLevel > 0)
			{
				player.memorizedSpells.push_back(key);
				--slotsAtLevel;
			}
		}
	}

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Spells memorized: ");
	for (size_t i = 0; i < player.memorizedSpells.size(); ++i)
	{
		if (i > 0)
		{
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", ");
		}
		ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, ctx.spellRegistry->get_by_key(player.memorizedSpells[i]).name);
	}
	ctx.messageSystem->finalize_message();
}

void SpellSystem::show_casting_menu(Player& player, GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuSpellCast>(player, ctx));
}

std::vector<SpellSystem::ItemGrantedSpell> SpellSystem::get_item_granted_spells(const Player& player)
{
	std::vector<ItemGrantedSpell> itemSpells;

	// Check for Ring of Invisibility, once however many are worn
	if (player.wears_ring_of(MagicalEffect::INVISIBILITY))
	{
		itemSpells.push_back({ "invisibility", "Ring" });
	}

	// Check for Helm of Teleportation
	if (Item* helm = player.get_equipped_item(EquipmentSlot::HEAD))
	{
		if (const auto* magicHelm = helm->behavior ? std::get_if<MagicalHelm>(&*helm->behavior) : nullptr)
		{
			if (magicHelm->effect == MagicalEffect::TELEPORTATION)
			{
				itemSpells.push_back({ "teleport", "Helm" });
			}
		}
	}

	return itemSpells;
}
