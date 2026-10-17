// file: Monsters.cpp
#include <cassert>
#include <memory>

#include "Actor.h"
#include "AiMimic.h"
#include "Colors.h"
#include "CreatureSize.h"
#include "DamageInfo.h"
#include "ExperienceReward.h"
#include "GameContext.h"
#include "IntelligenceRating.h"
#include "LevelUpSystem.h"
#include "MonsterAbilityTables.h"
#include "MonsterAttacker.h"
#include "MonsterRegistry.h"
#include "Monsters.h"
#include "RandomDice.h"
#include "Vector2D.h"

Mimic::Mimic(Vector2D position, GameContext& ctx)
	: Creature(position, ActorData{ ctx.monsterRegistry->get_tile(MonsterId::MIMIC), "mimic", ColorPairId::RED_YELLOW })
{
	// Monstrous Manual, the killer mimic (PDF page 1700): THAC0 11, AC 7.
	const int thaco = 11;
	const int ac = 7;

	// Tables 6 to 11: the killer mimic is large and semi-intelligent (Monstrous Manual
	// p1700). Before the hit points, which read the Constitution.
	assert(ctx.dice && "Mimic constructed without dice");
	roll_monster_ability_scores(*this, CreatureSize::LARGE, IntelligenceRating::SEMI, *ctx.dice);

	set_natural_attack("Pseudopod");

	// Monstrous Manual, the mimic entry's second column: the killer mimic, which is
	// what an ambusher that never bargains is. Neutral (evil).
	set_ethics(Ethics::NEUTRAL);
	set_morality(Morality::EVIL);

	// Damage/Attack 3-12 (smash), which is 3d4.
	attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{ "3d4", DamageType::PHYSICAL });
	set_dr(1);
	set_thaco(thaco);
	armorClass = std::make_unique<ArmorClass>(ac);

	// Hit Dice 9-10: the number is its level, each d8 carries its Constitution, and its
	// XP Value is the book's for that many, 2,000 at 9 and 3,000 at 10. Rolled after
	// the score the dice carry.
	assert(ctx.dataManager && "Mimic built without a dataManager");
	const int hitDice = ctx.dice->roll(9, 10);
	set_creature_level(hitDice);
	set_hit_dice(LevelUpSystem::roll_hit_points(
		DiceExpr{ hitDice, 8, 0 },
		get_creature_class(),
		get_constitution(),
		*ctx.dataManager,
		*ctx.dice));
	experienceReward = std::make_unique<ExperienceReward>(hitDice == 9 ? 2000 : 3000);

	// Build disguise list -- single source of truth is in AiMimic (Appearance::build_mimic_list).
	auto disguises = Appearance::build_mimic_list(*ctx.contentRegistry, *ctx.itemRegistry);

	assert(!disguises.empty() && "Mimic: the content gave a mimic no appearance to wear");

	// Apply initial random disguise to this creature's visible appearance.
	const size_t index = ctx.dice->roll(0, static_cast<int>(disguises.size()) - 1);
	const auto& chosen = disguises.at(index);
	actorData.tile = chosen.tile;
	actorData.name = chosen.name;
	actorData.color = chosen.color;

	// Transfer ownership of the disguise list to AiMimic.
	ai = std::make_unique<AiMimic>(std::move(disguises));

	remove_state(ActorState::BLOCKS);

	assert(ai && "Mimic requires Ai");
	assert(attacker && "Mimic requires Attacker");
}
