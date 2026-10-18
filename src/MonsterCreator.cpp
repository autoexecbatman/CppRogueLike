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
#include "ItemCreator.h"
#include "LevelUpSystem.h"
#include "MonsterAbilityTables.h"
#include "MonsterAttacker.h"
#include "MonsterCreator.h"
#include "MonsterHitDice.h"
#include "MonsterRegistry.h"
#include "Vector2D.h"

std::unique_ptr<Creature> MonsterCreator::create_from_params(
	Vector2D pos,
	const MonsterParams& params,
	GameContext& ctx)
{
	assert(ctx.dice && "MonsterCreator::create_from_params: called without dice");

	auto c = std::make_unique<Creature>(pos, ActorData{ params.symbol, params.name, params.color });

	// All six from the DMG's tables by size and rating, with the Strength its entry prints.
	roll_monster_ability_scores(*c, params.size, params.intelligence, params.strength, *ctx.dice);

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
	// One count needs no choice and spends no die; a range rolls its count, and the level,
	// the hit points and the experience all follow that roll.
	assert(is_valid_monster_hit_dice(params.hitDice) && "MonsterCreator::create_from_params: hit dice the loader would refuse");
	const HitDiceCount& lowest = params.hitDice.counts.front();
	const int count = params.hitDice.counts.size() == 1
		? lowest.count
		: ctx.dice->roll(lowest.count, highest_count(params.hitDice).count);
	const HitDiceCount& rolled = count_entry(params.hitDice, count);
	c->set_creature_level(rolled.count);

	// Rolled after the scores are set, because each die carries the Constitution.
	assert(ctx.dataManager && "MonsterCreator::create_from_params called without a dataManager");
	const int hp = LevelUpSystem::roll_hit_points(
		dice_for(params.hitDice, rolled.count),
		c->get_creature_class(),
		c->get_constitution(),
		*ctx.dataManager,
		*ctx.dice);

	c->attacker = std::make_unique<MonsterAttacker>(*c, params.damage);
	c->experienceReward = std::make_unique<ExperienceReward>(rolled.xp);
	c->set_dr(params.dr);
	c->set_thaco(params.thaco);
	c->armorClass = std::make_unique<ArmorClass>(params.ac);
	c->set_hit_dice(hp);

	// The Ai its record names; the loader admits only those a record may name.
	assert(is_authorable_ai_type(params.aiType) && "MonsterCreator::create_from_params: a record named an Ai no record may name");
	c->ai = Ai::make(params.aiType);

	if (params.canSwim)
	{
		c->add_state(ActorState::CAN_SWIM);
	}

	if (params.canFly)
	{
		c->add_state(ActorState::CAN_FLY);
	}

	if (params.walksWebs)
	{
		c->add_state(ActorState::CAN_WALK_WEBS);
	}

	c->set_attitude(params.attitude);
	c->set_displaceable(params.displaceable);

	assert(c->ai && "Monster requires Ai");
	assert(c->attacker && "Monster requires Attacker");

	// Last, so whatever its Ai does at birth sees the creature whole.
	c->ai->on_created(*c, ctx);

	return c;
}

std::unique_ptr<Creature> MonsterCreator::create(Vector2D pos, MonsterId id, GameContext& ctx)
{
	assert(ctx.monsterRegistry && "MonsterCreator::create: called without a monsterRegistry");

	return create_from_params(pos, ctx.monsterRegistry->get_standard_monsters().at(id), ctx);
}

// end of file: MonsterCreator.cpp
