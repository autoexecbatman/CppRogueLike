#pragma once

// file: CountingAi.h
//
// A mind that does nothing but count how many times it was asked to act. The turn
// order's claim is about how often a creature is driven, so what it does when driven
// is not the subject - and a real Ai would need a map, a Dijkstra field and a player
// to walk towards.
//
// Usage:
//
//   auto* mind = new CountingAi{};   // the creature takes ownership below
//   creature->ai.reset(mind);
//   manager.update_creatures(creatures, ctx);
//   EXPECT_EQ(mind->updates, 1);     // driven once in the window

#include "src/Ai.h"
#include "src/Creature.h"

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

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::MONSTER; }

	void load(const json& j) override { (void)j; }
	void save(json& j) override { (void)j; }
};

// end of file: CountingAi.h
