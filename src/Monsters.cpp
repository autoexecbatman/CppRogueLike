// file: Monsters.cpp
#include <cassert>
#include <memory>

#include "Actor.h"
#include "MonsterAttacker.h"
#include "AiMimic.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "ExperienceReward.h"
#include "GameContext.h"
#include "MonsterRegistry.h"
#include "RandomDice.h"
#include "Vector2D.h"
#include "Monsters.h"

Mimic::Mimic(Vector2D position, GameContext& ctx)
	: Creature(position, ActorData{ ctx.monsterRegistry->get_tile(MonsterId::MIMIC), "mimic", RED_YELLOW_PAIR })
{
	const int hp = ctx.dice->d6() + ctx.dice->d4();
	const int thaco = 17;
	const int ac = 7;

	// Mimic: AD&D 2e -- strong pseudopod, average dex, tough, low animal INT,
	// decent predator WIS, very low CHA (horrifying when revealed).
	set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6() + 2); // 3d6+2 avg 12
	set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());    // 3d6    avg 10
	set_constitution(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // 3d6    avg 10
	set_intelligence(ctx.dice->d4() + 2);                               // 1d4+2  avg  4
	set_wisdom(ctx.dice->d6() + ctx.dice->d6() + 1);                    // 2d6+1  avg  8
	set_charisma(ctx.dice->d4());                                        // 1d4    avg  2

	set_natural_attack("Pseudopod");

	// Monstrous Manual, the mimic entry's second column: the killer mimic, which is
	// what an ambusher that never bargains is. Neutral (evil).
	set_ethics(Ethics::NEUTRAL);
	set_morality(Morality::EVIL);

	attacker = std::make_unique<MonsterAttacker>(*this, DamageValues::Dagger());
	experienceReward = std::make_unique<ExperienceReward>(150);
	set_dr(1);
	set_thaco(thaco);
	armorClass = std::make_unique<ArmorClass>(ac);
	set_hit_dice(hp);

	// Build disguise list -- single source of truth is in AiMimic (Appearance::build_mimic_list).
	auto disguises = Appearance::build_mimic_list(*ctx.contentRegistry, *ctx.itemRegistry);

	if (disguises.empty())
	{
		throw("possibleDisguises is empty from Appearance::build_mimic_list()!");
	}

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
