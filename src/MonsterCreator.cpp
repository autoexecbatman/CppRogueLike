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
#include "SpellSystem.h"
#include "Vector2D.h"

std::unique_ptr<Creature> MonsterCreator::create_from_params(
	Vector2D pos,
	const MonsterParams& params,
	GameContext& ctx)
{
	assert(
		ctx.dice && "MonsterCreator::create_from_params: called without dice");

	auto creature = std::make_unique<Creature>(
		pos,
		ActorData{ params.symbol, params.name, params.color });

	// All six from the DMG's tables by size and rating, with the Strength its entry prints.
	roll_monster_ability_scores(
		*creature,
		params.size,
		params.intelligence,
		params.strength,
		*ctx.dice);

	creature->set_natural_attack(params.naturalAttack);
	assert(
		ctx.bodyPlanRegistry &&
		"MonsterCreator::create_from_params called without a bodyPlanRegistry");
	creature->set_body_plan(ctx.bodyPlanRegistry->get(params.bodyPlanName));

	// Whatever the creature carries is a real item, created through the same
	// path the player's gear comes from.
	assert(
		ctx.contentRegistry &&
		"MonsterCreator::create_from_params called without a contentRegistry");
	for (const MonsterParams::StartingItem& carried : params.equipment)
	{
		std::unique_ptr<Item> carriedItem =
			ItemCreator::create(carried.itemKey, pos, ctx);

		// Authored data, so a mismatch is a typo in the file rather than an
		// impossible state: it says which monster and which slot.
		if (!creature->can_equip(*carriedItem, carried.slot))
		{
			throw std::runtime_error(
				std::format(
					"MonsterCreator::create_from_params -- '{}' cannot carry '{}' in the {} slot",
					params.name,
					carried.itemKey,
					encode_equipment_slot(carried.slot)));
		}

		creature->wear(std::move(carriedItem), carried.slot);
	}
	creature->set_morale(params.morale);
	creature->set_undead(params.undead);
	creature->set_ethics(params.ethics);
	creature->set_morality(params.morality);
	creature->set_corpse_weight(params.corpseWeight);
	creature->set_corpse(
		params.corpseName,
		params.corpseNutrition,
		params.corpseTaste,
		params.corpseEdibility);
	// One count needs no choice and spends no die; a range rolls its count, and the level,
	// the hit points and the experience all follow that roll.
	assert(
		is_valid_monster_hit_dice(params.hitDice) &&
		"MonsterCreator::create_from_params: hit dice the loader would refuse");
	const HitDiceCount& lowest = params.hitDice.counts.front();
	const int count = params.hitDice.counts.size() == 1 ?
		lowest.count :
		ctx.dice->roll(lowest.count, highest_count(params.hitDice).count);
	const HitDiceCount& rolled = count_entry(params.hitDice, count);
	creature->set_creature_level(rolled.count);

	// Rolled after the scores are set, because each die carries the Constitution.
	assert(
		ctx.dataManager &&
		"MonsterCreator::create_from_params called without a dataManager");
	const int hp = LevelUpSystem::roll_hit_points(
		dice_for(params.hitDice, rolled.count),
		creature->get_creature_class(),
		creature->get_constitution(),
		*ctx.dataManager,
		*ctx.dice);

	creature->attacker =
		std::make_unique<MonsterAttacker>(*creature, params.damage);
	creature->experienceReward = std::make_unique<ExperienceReward>(rolled.xp);
	creature->set_dr(params.dr);
	creature->set_thaco(params.thaco);
	creature->armorClass = std::make_unique<ArmorClass>(params.ac);
	creature->set_hit_dice(hp);

	// The Ai its record names; the loader admits only those a record may name.
	assert(
		is_authorable_ai_type(params.aiType) &&
		"MonsterCreator::create_from_params: a record named an Ai no record may name");
	creature->ai = Ai::make(params.aiType);

	if (params.canSwim)
	{
		creature->add_state(ActorState::CAN_SWIM);
	}

	if (params.canFly)
	{
		creature->add_state(ActorState::CAN_FLY);
	}

	if (params.walksWebs)
	{
		creature->add_state(ActorState::CAN_WALK_WEBS);
	}

	creature->set_attitude(params.attitude);
	creature->set_displaceable(params.displaceable);

	// Its record's casting, then the same fill the player memorizes by, from its rolled Wisdom.
	creature->spellcasting.training = params.spellcasterTraining;
	creature->spellcasting.innatePowers = params.innatePowers;
	assert(
		ctx.spellRegistry &&
		"MonsterCreator::create_from_params called without a spellRegistry");
	SpellSystem::memorize_spells(
		*creature,
		*ctx.spellRegistry,
		*ctx.dataManager);

	assert(creature->ai && "Monster requires Ai");
	assert(creature->attacker && "Monster requires Attacker");

	// Last, so whatever its Ai does at birth sees the creature whole.
	creature->ai->on_created(*creature, ctx);

	return creature;
}

std::unique_ptr<Creature> MonsterCreator::create(
	Vector2D pos,
	MonsterId id,
	GameContext& ctx)
{
	assert(
		ctx.monsterRegistry &&
		"MonsterCreator::create: called without a monsterRegistry");

	return create_from_params(
		pos,
		ctx.monsterRegistry->get_standard_monsters().at(id),
		ctx);
}

// end of file: MonsterCreator.cpp
