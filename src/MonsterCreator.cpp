// file: MonsterCreator.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <stdexcept>

#include "AiMonster.h"
#include "AiMonsterRanged.h"
#include "BodyPlanRegistry.h"
#include "Creature.h"
#include "DamageInfo.h"
#include "ExperienceReward.h"
#include "GameContext.h"
#include "HealthPool.h"
#include "ItemCreator.h"
#include "MonsterAttacker.h"
#include "MonsterCreator.h"
#include "MonsterRegistry.h"
#include "RandomDice.h"
#include "Vector2D.h"

std::unique_ptr<Creature> MonsterCreator::create_from_params(
	Vector2D pos,
	const MonsterParams& params,
	GameContext& ctx)
{
	auto c = std::make_unique<Creature>(pos, ActorData{ params.symbol, params.name, params.color });

	c->set_strength(std::max(1, roll_dice(ctx.dice, params.strDice)));
	c->set_dexterity(std::max(1, roll_dice(ctx.dice, params.dexDice)));
	c->set_constitution(std::max(1, roll_dice(ctx.dice, params.conDice)));
	c->set_intelligence(std::max(1, roll_dice(ctx.dice, params.intDice)));
	c->set_wisdom(std::max(1, roll_dice(ctx.dice, params.wisDice)));
	c->set_charisma(std::max(1, roll_dice(ctx.dice, params.chaDice)));

	c->set_natural_attack(params.naturalAttack);
	assert(ctx.bodyPlanRegistry && "MonsterCreator::create_from_params called without a bodyPlanRegistry");
	c->set_body_plan(ctx.bodyPlanRegistry->get(params.bodyPlanName));

	// Whatever the creature carries is a real item, created through the same
	// path the player's gear comes from.
	assert(ctx.contentRegistry && "MonsterCreator::create_from_params called without a contentRegistry");
	for (const MonsterParams::StartingItem& carried : params.equipment)
	{
		std::unique_ptr<Item> carriedItem = ItemCreator::create(carried.itemKey, pos, ctx);

		// Authored data, so a mismatch is a typo in the file rather than an
		// impossible state: it says which monster and which slot.
		if (!c->can_equip(*carriedItem, carried.slot))
		{
			throw std::runtime_error(std::format(
				"MonsterCreator::create_from_params -- '{}' cannot carry '{}' in the {} slot",
				params.name,
				carried.itemKey,
				encode_equipment_slot(carried.slot)));
		}

		c->wear(std::move(carriedItem), carried.slot);
	}
	c->set_morale(params.morale);
	c->set_undead(params.undead);
	c->set_ethics(params.ethics);
	c->set_morality(params.morality);
	c->set_corpse_weight(params.corpseWeight);
	c->set_creature_level(params.hpDice.num);

	const int hp = std::max(1, roll_dice(ctx.dice, params.hpDice));

	c->attacker = std::make_unique<MonsterAttacker>(*c, params.damage);
	c->experienceReward = std::make_unique<ExperienceReward>(params.xp);
	c->set_dr(params.dr);
	c->set_thaco(params.thaco);
	c->armorClass = std::make_unique<ArmorClass>(params.ac);
	c->set_hit_dice(hp);

	if (params.aiType == MonsterAiType::RANGED)
	{
		c->ai = std::make_unique<AiMonsterRanged>();
	}
	else
	{
		c->ai = std::make_unique<AiMonster>();
	}

	if (params.canSwim)
	{
		c->add_state(ActorState::CAN_SWIM);
	}

	assert(c->ai && "Monster requires Ai");
	assert(c->attacker && "Monster requires Attacker");

	return c;
}

std::unique_ptr<Creature> MonsterCreator::create(Vector2D pos, MonsterId id, GameContext& ctx)
{
	return create_from_params(pos, ctx.monsterRegistry->get_standard_monsters().at(id), ctx);
}

// end of file: MonsterCreator.cpp
