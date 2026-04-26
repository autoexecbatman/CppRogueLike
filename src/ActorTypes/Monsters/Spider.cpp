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

#include "../../Actor/Actor.h"
#include "../../Actor/MonsterAttacker.h"
#include "../../Actor/Destructible.h"
#include "../../Ai/AiSpider.h"
#include "../../Ai/AiWebSpinner.h"
#include "../../Colors/Colors.h"
#include "../../Combat/DamageInfo.h"
#include "../../Combat/ExperienceReward.h"
#include "../../Core/GameContext.h"
#include "../../Factories/MonsterCreator.h"
#include "../../Random/RandomDice.h"
#include "../../Utils/Vector2D.h"
#include "Spider.h"

constexpr int POISON_CHANCE_SMALL_SPIDER = 25;
constexpr int POISON_CHANCE_GIANT_SPIDER = 15;
constexpr int POISON_CHANCE_WEB_SPINNER = 15;

// Base Spider constructor
Spider::Spider(Vector2D position, GameContext& ctx, SpiderType type)
	: Creature(position, ActorData{ MonsterCreator::get_tile(MonsterId::SPIDER_SMALL), "small spider", GREEN_BLACK_PAIR }), // Default to small spider data
	  spiderType(type)
{
	// Initialize based on spider type
	init_spider_type(ctx);
}

void Spider::init_spider_type(GameContext& ctx)
{
	// Common spider traits
	add_state(ActorState::CAN_SWIM); // Spiders can walk on water (they're light)

	switch (spiderType)
	{
	case SpiderType::SMALL:
		// Update actor data for small spider
		actorData = ActorData{ MonsterCreator::get_tile(MonsterId::SPIDER_SMALL), "small spider", GREEN_BLACK_PAIR };

		// Stats for small spider
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // Minimum strength of 3
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // Small spiders are very agile
		set_constitution(ctx.dice->d6());

		// Combat properties - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(45); // TRIPLED from 15 for solo play bonus
		destructible = std::make_unique<Destructible>(ctx.dice->d2() + 2, 0, "dead small spider", 0, 20, 7, std::make_unique<MonsterDeathHandler>());
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ 1, 4, "1d4" });
		set_weapon_equipped("Venomous fangs");

		ai = std::make_unique<AiSpider>(POISON_CHANCE_SMALL_SPIDER);
		break;

	case SpiderType::GIANT:
		// Update actor data for giant spider
		actorData = ActorData{ MonsterCreator::get_tile(MonsterId::SPIDER_GIANT), "giant spider", RED_BLACK_PAIR };

		// Stats for giant spider
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_constitution(ctx.dice->d6() + 1);

		// Combat properties - giant spiders have more HP and do more damage - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(120); // TRIPLED from 40 for solo play bonus
		destructible = std::make_unique<Destructible>(
			ctx.dice->d4() + 3,
			1,
			"dead giant spider",
			0,
			19,
			5,
			std::make_unique<MonsterDeathHandler>());
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ 1, 6, "1d6" });
		set_weapon_equipped("Giant fangs");

		ai = std::make_unique<AiSpider>(POISON_CHANCE_GIANT_SPIDER);
		break;

	case SpiderType::WEB_SPINNER:
		// Update actor data for web spinner
		actorData = ActorData{ MonsterCreator::get_tile(MonsterId::SPIDER_WEAVER), "web weaver", BLACK_GREEN_PAIR };

		// Stats for web spinner - now much more formidable
		set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());
		set_constitution(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());

		// Combat properties - significantly stronger - TRIPLED XP for solo play
		experienceReward = std::make_unique<ExperienceReward>(180); // TRIPLED from 60 for solo play bonus
		destructible = std::make_unique<Destructible>(ctx.dice->d8() + 5, 1, "dead web weaver", 0, 17, 5, std::make_unique<MonsterDeathHandler>());
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ 1, 8, "1d8" });
		set_weapon_equipped("Toxic fangs");

		ai = std::make_unique<AiWebSpinner>(POISON_CHANCE_WEB_SPINNER);
		break;
	}

	assert(ai && "Spider requires Ai");
	assert(attacker && "Spider requires Attacker");
	assert(destructible && "Spider requires Destructible");
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