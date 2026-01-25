#include <iostream>
#include <memory>

#include "../Actor/Actor.h"
#include "Ai.h"
#include "AiMonsterConfused.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"

//==ConfusedMonsterAi==
AiMonsterConfused::AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept : nbTurns{nbTurns}, oldAi{std::move(oldAi)} {}

void AiMonsterConfused::update(Creature& owner, GameContext& ctx)
{
    const Vector2D direction = get_random_direction(ctx);
    
    // Only move if we got a valid direction (not {0, 0})
    if (direction != Vector2D{0, 0})
    {
        const Vector2D destination = owner.position + direction;
        attempt_move(owner, destination, ctx);
    }
    
    // Decrement turns and check if confusion ended
    --nbTurns;
    if (nbTurns <= 0)
    {
        restore_original_ai(owner);
    }
}

[[nodiscard]] Vector2D AiMonsterConfused::get_random_direction(GameContext& ctx) const
{
    return Vector2D{
        ctx.dice->roll(MIN_DIRECTION, MAX_DIRECTION),
        ctx.dice->roll(MIN_DIRECTION, MAX_DIRECTION)
    };
}

void AiMonsterConfused::attempt_move(Creature& owner, const Vector2D& destination, GameContext& ctx)
{
    if (ctx.map->can_walk(destination, ctx))
    {
        owner.position = destination;
    }
    else
    {
        // Try to attack whatever is blocking the way
        const auto& actor = ctx.map->get_actor(destination, ctx);
        if (actor && owner.attacker)
        {
            owner.attacker->attack(owner, *actor, ctx);
        }
    }
}

void AiMonsterConfused::restore_original_ai(Creature& owner)
{
    if (oldAi)
    {
        owner.ai = std::move(oldAi);
    }
}

void AiMonsterConfused::load(const json& j)
{
	nbTurns = j.at("nbTurns").get<int>();

	// Create the oldAi if it exists in the JSON
	if (j.contains("oldAi"))
	{
		oldAi = Ai::create(j["oldAi"]);
	}
}

void AiMonsterConfused::save(json& j)
{
	j["type"] = static_cast<int>(AiType::CONFUSED_MONSTER);
	j["nbTurns"] = nbTurns;

	// Save the oldAi if it exists
	if (oldAi != nullptr)
	{
		json oldAiJson;
		oldAi->save(oldAiJson);
		j["oldAi"] = oldAiJson;
	}
}