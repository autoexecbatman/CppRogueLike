// file: Monsters.cpp
#include <cassert>
#include <memory>

#include "../Actor/Actor.h"
#include "../Actor/MonsterAttacker.h"
#include "../Actor/Destructible.h"
#include "../Ai/AiMimic.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "../Factories/MonsterCreator.h"
#include "../Random/RandomDice.h"
#include "../Systems/ContentRegistry.h"
#include "../Utils/Vector2D.h"
#include "Monsters.h"

Mimic::Mimic(Vector2D position, GameContext& ctx)
	: Creature(position, ActorData{ MonsterCreator::get_tile(MonsterId::MIMIC), "mimic", RED_YELLOW_PAIR })
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

	set_weapon_equipped("Pseudopod");

	attacker = std::make_unique<MonsterAttacker>(*this, DamageValues::Dagger());
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead mimic", 150, thaco, ac);

	// Build disguise list -- single source of truth is in AiMimic (Appearance::build_mimic_list).
	auto disguises = Appearance::build_mimic_list(*ctx.contentRegistry);

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
	assert(destructible && "Mimic requires Destructible");
}
