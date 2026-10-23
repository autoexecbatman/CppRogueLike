#include <cassert>
#include <memory>
#include <utility>

#include "Ai.h"
#include "AiMonsterConfused.h"
#include "AttackKind.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Persistent.h"
#include "RandomDice.h"
#include "Vector2D.h"

constexpr int MIN_DIRECTION = -1;
constexpr int MAX_DIRECTION = 1;

//==ConfusedMonsterAi==
AiMonsterConfused::AiMonsterConfused(
	int confusionEndTime,
	std::unique_ptr<Ai> oldAi) noexcept :
	confusionEndTime{ confusionEndTime },
	oldAi{ std::move(oldAi) }
{
}

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

[[nodiscard]] Vector2D AiMonsterConfused::get_random_direction(
	GameContext& ctx) const
{
	return Vector2D{
		ctx.dice->roll(MIN_DIRECTION, MAX_DIRECTION),
		ctx.dice->roll(MIN_DIRECTION, MAX_DIRECTION),
	};
}

void AiMonsterConfused::attempt_move(
	Creature& owner,
	const Vector2D& destination,
	GameContext& ctx)
{
	assert(ctx.map && "AiMonsterConfused::attempt_move: called without a map");

	// Whatever stands in the way is struck instead.
	const auto& actor = ctx.map->get_actor(destination, ctx);
	if (actor && owner.attacker)
	{
		owner.attacker->attack(*actor, AttackKind::MELEE, ctx);
		return;
	}

	// A confused creature still opens a door it bumps and still springs a trap.
	Pathfinding::step(owner, destination, ctx);
}

void AiMonsterConfused::restore_original_ai(Creature& owner)
{
	assert(
		oldAi &&
		"AiMonsterConfused::restore_original_ai: a confused mind wraps no mind to return to");
	// Destroys this object, whose oldAi has just been moved out.
	owner.ai = std::move(oldAi);
}

void AiMonsterConfused::load(const json& source)
{
	confusionEndTime = source.at("confusionEndTime").get<int>();

	oldAi = Ai::create(source.at("oldAi"));
}

void AiMonsterConfused::save(json& output)
{
	output["type"] = encode_ai_type(get_ai_type());
	output["confusionEndTime"] = confusionEndTime;

	assert(
		oldAi &&
		"AiMonsterConfused::save: a confused mind wraps no mind to return to");
	json oldAiJson;
	oldAi->save(oldAiJson);
	output["oldAi"] = oldAiJson;
}