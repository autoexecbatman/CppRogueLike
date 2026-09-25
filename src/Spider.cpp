/*
 * SPIDER SOLO AD&D 2E XP BONUS SYSTEM
 * ===================================
 *
 * All spider XP values have been TRIPLED from standard values to maintain
 * consistency with the solo play bonus system implemented across all monsters.
 *
 * STANDARD vs SOLO XP VALUES:
 * - Small Spider:    15 XP  → 45 XP   (3x multiplier)
 * - Giant Spider:    40 XP  → 120 XP  (3x multiplier)
 * - Web Weaver:      60 XP  → 180 XP  (3x multiplier)
 *
 * This ensures spiders provide equivalent progression rewards as other
 * creatures in the solo AD&D 2e experience system.
 */
#include <cassert>
#include <memory>

#include "Actor.h"
#include "MonsterAttacker.h"
#include "AiGiantSpider.h"
#include "AiSpider.h"
#include "AiWebSpinner.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "DataManager.h"
#include "ExperienceReward.h"
#include "GameContext.h"
#include "LevelUpSystem.h"
#include "MonsterRegistry.h"
#include "RandomDice.h"
#include "Vector2D.h"
#include "Spider.h"

constexpr int POISON_CHANCE_SMALL_SPIDER = 25;
constexpr int POISON_CHANCE_GIANT_SPIDER = 15;
constexpr int POISON_CHANCE_WEB_SPINNER = 15;

// Base Spider constructor
Spider::Spider(Vector2D position, GameContext& ctx, SpiderType type)
	: Creature(position, ActorData{ ctx.monsterRegistry->get_tile(MonsterId::SPIDER_SMALL), "small spider", GREEN_BLACK_PAIR }), // Default to small spider data
	  spiderType(type)
{
	// Initialize based on spider type
	init_spider_type(ctx);
}

void Spider::init_spider_type(GameContext& ctx)
{
	// Common spider traits
	add_state(ActorState::CAN_SWIM); // Spiders can walk on water (they're light)
	add_state(ActorState::CAN_WALK_WEBS); // Spiders are not caught by webs, their own or others'

	// Monstrous Manual, the spider entry, in its column order: the hairy spider is
	// neutral evil, the huge one neutral, and the giant one chaotic evil. Which of
	// them each of these is was settled with their venom.
	switch (spiderType)
	{
	case SpiderType::SMALL:
	{
		set_ethics(Ethics::NEUTRAL);
		set_morality(Morality::EVIL);
		break;
	}
	case SpiderType::GIANT:
	{
		set_ethics(Ethics::NEUTRAL);
		set_morality(Morality::NEUTRAL);
		break;
	}
	case SpiderType::WEB_SPINNER:
	{
		set_ethics(Ethics::CHAOTIC);
		set_morality(Morality::EVIL);
		break;
	}
	}

	// Called after each variant's score is set, because every hit die carries it.
	assert(ctx.dataManager && "Spider built without a dataManager");
	const auto roll_hit_dice = [this, &ctx](int sides, int bonus)
	{
		return LevelUpSystem::roll_hit_points(
			DiceExpr{ 1, sides, bonus },
			get_creature_class(),
			get_constitution(),
			*ctx.dataManager,
			*ctx.dice);
	};

	switch (spiderType)
	{
	case SpiderType::SMALL:
		// Update actor data for small spider
		actorData = ActorData{ ctx.monsterRegistry->get_tile(MonsterId::SPIDER_SMALL), "small spider", GREEN_BLACK_PAIR };

		// Stats for small spider
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // Minimum strength of 3
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // Small spiders are very agile
		set_constitution(ctx.dice->d6());

		// Combat properties - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(45); // TRIPLED from 15 for solo play bonus
		set_dr(0);
		set_thaco(20);
		armorClass = std::make_unique<ArmorClass>(7);
		set_hit_dice(roll_hit_dice(2, 2));
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ "1d4", DamageType::PHYSICAL });
		set_natural_attack("Venomous fangs");

		ai = std::make_unique<AiSpider>(POISON_CHANCE_SMALL_SPIDER);
		break;

	case SpiderType::GIANT:
		// Update actor data for giant spider
		actorData = ActorData{ ctx.monsterRegistry->get_tile(MonsterId::SPIDER_GIANT), "giant spider", RED_BLACK_PAIR };

		// Stats for giant spider
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_constitution(ctx.dice->d6() + 1);

		// Combat properties - giant spiders have more HP and do more damage - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(120); // TRIPLED from 40 for solo play bonus
		set_dr(1);
		set_thaco(19);
		armorClass = std::make_unique<ArmorClass>(5);
		set_hit_dice(roll_hit_dice(4, 3));
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ "1d6", DamageType::PHYSICAL });
		set_natural_attack("Giant fangs");

		ai = std::make_unique<AiGiantSpider>(POISON_CHANCE_GIANT_SPIDER);
		break;

	case SpiderType::WEB_SPINNER:
		// Update actor data for web spinner
		actorData = ActorData{ ctx.monsterRegistry->get_tile(MonsterId::SPIDER_WEAVER), "web weaver", BLACK_GREEN_PAIR };

		// Stats for web spinner - now much more formidable
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_constitution(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());

		// Combat properties - significantly stronger - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(180); // TRIPLED from 60 for solo play bonus
		set_dr(1);
		set_thaco(17);
		armorClass = std::make_unique<ArmorClass>(5);
		set_hit_dice(roll_hit_dice(8, 5));
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ "1d8", DamageType::PHYSICAL });
		set_natural_attack("Toxic fangs");

		ai = std::make_unique<AiWebSpinner>(POISON_CHANCE_WEB_SPINNER);
		break;
	}

	assert(ai && "Spider requires Ai");
	assert(attacker && "Spider requires Attacker");
}

void Spider::update(GameContext& ctx)
{
	// Call the base class update method
	Creature::update(ctx);
}

// Small Spider implementation
SmallSpider::SmallSpider(Vector2D position, GameContext& ctx)
	: Spider(position, ctx, SpiderType::SMALL)
{
	// Any additional small spider initialization
}

// Giant Spider implementation
GiantSpider::GiantSpider(Vector2D position, GameContext& ctx)
	: Spider(position, ctx, SpiderType::GIANT)
{
	// Any additional giant spider initialization
}

// Web Spinner implementation
WebSpinner::WebSpinner(Vector2D position, GameContext& ctx)
	: Spider(position, ctx, SpiderType::WEB_SPINNER)
{
	// Any additional web spinner initialization
}