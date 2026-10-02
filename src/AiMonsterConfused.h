#pragma once

#include <memory>

#include "Ai.h"
#include "Persistent.h"

class Creature;
struct GameContext;
struct Vector2D;

class AiMonsterConfused : public Ai
{
public:
	// Takes the clock reading the confusion ends at rather than a count of rounds: the
	// creature's own update runs once per action, so a count would fall twice as fast
	// for a creature twice as quick. Creature::apply_confusion works the reading out.
	AiMonsterConfused(int confusionEndTime, std::unique_ptr<Ai> oldAi) noexcept;
	~AiMonsterConfused() override = default;

	AiMonsterConfused(const AiMonsterConfused&) = delete;
	AiMonsterConfused& operator=(const AiMonsterConfused&) = delete;
	AiMonsterConfused(AiMonsterConfused&&) noexcept = delete;
	AiMonsterConfused& operator=(AiMonsterConfused&&) noexcept = delete;

	void update(Creature& owner, GameContext& ctx) override;

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::CONFUSED_MONSTER; }
	void load(const json& j) override;
	void save(json& j) override;

private:
	int confusionEndTime{ 0 };
	std::unique_ptr<Ai> oldAi;

	[[nodiscard]] Vector2D get_random_direction(GameContext& ctx) const;
	void attempt_move(Creature& owner, const Vector2D& destination, GameContext& ctx);
	void restore_original_ai(Creature& owner);
};
