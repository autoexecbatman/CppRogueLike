#pragma once

#include <memory>

#include "Ai.h"

class Creature;
struct GameContext;
struct Vector2D;

class AiMonsterConfused : public Ai
{
public:
	AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept;

    AiMonsterConfused(const AiMonsterConfused&) = delete;
    AiMonsterConfused& operator=(const AiMonsterConfused&) = delete;
    AiMonsterConfused(AiMonsterConfused&&) noexcept = delete;
    AiMonsterConfused& operator=(AiMonsterConfused&&) noexcept = delete;

	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;

private:
	static constexpr int MIN_DIRECTION = -1;
    static constexpr int MAX_DIRECTION = 1;

	int nbTurns;
	std::unique_ptr<Ai> oldAi;

	[[nodiscard]] Vector2D get_random_direction(GameContext& ctx) const;
	void attempt_move(Creature& owner, const Vector2D& destination, GameContext& ctx);
	void restore_original_ai(Creature& owner);
};
