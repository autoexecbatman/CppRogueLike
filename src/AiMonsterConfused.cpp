#include <memory>
#include <utility>

#include "Ai.h"
#include "AiMonsterConfused.h"
#include "AttackKind.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "Persistent.h"
#include "RandomDice.h"
#include "Vector2D.h"

constexpr int MIN_DIRECTION = -1;
constexpr int MAX_DIRECTION = 1;

//==ConfusedMonsterAi==
AiMonsterConfused::AiMonsterConfused(int confusionEndTime, std::unique_ptr<Ai> oldAi) noexcept
	: confusionEndTime{ confusionEndTime }, oldAi{ std::move(oldAi) } {}

void AiMonsterConfused::update(Creature& owner, GameContext& ctx)
{
	const Vector2D direction = get_random_direction(ctx);

	// Only move if we got a valid direction (not {0, 0})
	if (direction != Vector2D{ 0, 0 })
	{
		const Vector2D destination = owner.position + direction;
		attempt_move(owner, destination, ctx);
	}

	// The spell is over when the clock says so. Asked rather than counted, so a creature
	// that acts twice in a round is confused for the rounds the spell named.
	if (ctx.gameState->get_time() >= confusionEndTime)
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
			owner.attacker->attack(*actor, AttackKind::MELEE, ctx);
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
	confusionEndTime = j.at("confusionEndTime").get<int>();

	// Create the oldAi if it exists in the JSON
	if (j.contains("oldAi"))
	{
		oldAi = Ai::create(j["oldAi"]);
	}
}

void AiMonsterConfused::save(json& j)
{
	j["type"] = encode_ai_type(get_ai_type());
	j["confusionEndTime"] = confusionEndTime;

	// Save the oldAi if it exists
	if (oldAi != nullptr)
	{
		json oldAiJson;
		oldAi->save(oldAiJson);
		j["oldAi"] = oldAiJson;
	}
}