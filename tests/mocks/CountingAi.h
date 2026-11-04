#pragma once

// file: CountingAi.h
//
// A mind that does nothing but count how many times it was asked to act. The turn
// order's claim is about how often a creature is driven, so what it does when driven
// is not the subject - and a real Ai would need a map, a cost map and a player
// to walk towards.
//
// Usage:
//
//   auto* mind = new CountingAi{};   // the creature takes ownership below
//   creature->ai.reset(mind);
//   manager.update_creatures(creatures, ctx);
//   EXPECT_EQ(mind->updates, 1);     // driven once in the window

#include <nlohmann/json_fwd.hpp>

#include "src/Ai.h"
#include "src/Creature.h"

namespace CppRL
{

	class CountingAi : public Ai
	{
	public:
		int updates{ 0 };

		void update(Creature& owner, GameContext& ctx) override
		{
			(void)owner;
			(void)ctx;
			++updates;
		}

		[[nodiscard]] AiType get_ai_type() const noexcept override
		{
			return AiType::MONSTER;
		}

		void load([[maybe_unused]] const nlohmann::json& source) override {}

		void save([[maybe_unused]] nlohmann::json& output) const override {}
	};

	// end of file: CountingAi.h

} // namespace CppRL
