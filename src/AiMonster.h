#pragma once

#include <nlohmann/json_fwd.hpp>

#include "Ai.h"

namespace CppRL
{

	class Creature;
	struct GameContext;
	struct Vector2D;

	class AiMonster : public Ai
	{
	private:
		void update_tracking(Creature& owner, const GameContext& ctx);
		void decide_action(Creature& owner, GameContext& ctx);

	protected:
		virtual void move_or_attack(
			Creature& owner,
			Creature& target,
			GameContext& ctx);

	public:
		void update(Creature& owner, GameContext& ctx) override;

		[[nodiscard]] AiType get_ai_type() const noexcept override
		{
			return AiType::MONSTER;
		}

		void load(const nlohmann::json& source) override;
		void save(nlohmann::json& output) const override;
	};

} // namespace CppRL
