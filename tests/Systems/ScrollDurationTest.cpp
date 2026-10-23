// file: ScrollDurationTest.cpp
// How long a scroll that acts on everything the reader sees keeps its effect on each
// creature, rolled once at the reading and counted in the unit the record names.
//
// - Sleep and hold person carry the game's own fixed rounds, 5 and 8, read from the data.
// - A scroll authored in turns lasts ten rounds a turn: "Ten combat rounds equal a turn"
//   (PHB, PDF page 184).
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ScrollDurationTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <variant>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DiceExpr.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/TargetMode.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// The book's turn, in its rounds: "Ten combat rounds equal a turn" (PHB, PDF page 184).
constexpr int ROUNDS_IN_A_TURN = 10;
// The d20 that fails any saving throw, so every creature in view takes the effect.
constexpr int FAILED_SAVE = 1;
} // namespace

class ScrollDurationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.player = std::make_unique<Player>(Vector2D{ 4, 5 });
		mock.player->experienceReward = std::make_unique<ExperienceReward>(0);
		mock.player->healthPool = std::make_unique<HealthPool>(10);
		mock.player->armorClass = std::make_unique<ArmorClass>(10);

		// An open corridor, so the goblin beside the reader is in view.
		mock.map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			mock.map.set_tile(Vector2D{ column, 5 }, TileType::FLOOR);
		}

		auto goblin = std::make_unique<Creature>(
			Vector2D{ 5, 5 },
			ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
		goblin->healthPool = std::make_unique<HealthPool>(10);
		goblin->armorClass = std::make_unique<ArmorClass>(10);
		target = goblin.get();
		mock.creatures.push_back(std::move(goblin));

		mock.map.compute_fov(ctx);
		ASSERT_TRUE(mock.map.is_in_fov(target->position))
			<< "the goblin is not in view of the reader";
	}

	// Reads the scroll with the goblin's save failed, and returns the effect it grants.
	BuffType read(Item& scroll)
	{
		const BuffType granted =
			std::get<TargetedScroll>(*scroll.behavior).buffType;
		mock.dice.set_next_roll(FAILED_SAVE);
		use_item(*scroll.behavior, scroll, *mock.player, ctx);
		return granted;
	}

	// Whether the goblin still has the effect once the clock reads `rounds` after the reading.
	bool lasts_to(BuffType granted, int rounds)
	{
		ctx.buffSystem->update_creature_buffs(*target, expiry_time(0, rounds));
		return ctx.buffSystem->has_buff(*target, granted);
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	Creature* target{ nullptr };
};

TEST_F(ScrollDurationTest, SleepLastsItsFiveRounds)
{
	auto scroll =
		ItemCreator::create("scroll_sleep", mock.player->position, ctx);
	const BuffType granted = read(*scroll);

	EXPECT_TRUE(lasts_to(granted, 4));
	EXPECT_FALSE(lasts_to(granted, 5));
}

TEST_F(ScrollDurationTest, HoldPersonLastsItsEightRounds)
{
	auto scroll =
		ItemCreator::create("scroll_hold_person", mock.player->position, ctx);
	const BuffType granted = read(*scroll);

	EXPECT_TRUE(lasts_to(granted, 7));
	EXPECT_FALSE(lasts_to(granted, 8));
}

// The same scroll authored in turns: two turns are twenty rounds, not two.
TEST_F(ScrollDurationTest, AScrollAuthoredInTurnsLastsTenRoundsATurn)
{
	auto scroll =
		ItemCreator::create("scroll_sleep", mock.player->position, ctx);
	TargetedScroll& sleep = std::get<TargetedScroll>(*scroll->behavior);
	sleep.duration = DiceExpr{ 0, 0, 2 };
	sleep.durationUnit = DurationUnit::TURNS;
	const BuffType granted = read(*scroll);

	EXPECT_TRUE(lasts_to(granted, 2 * ROUNDS_IN_A_TURN - 1));
	EXPECT_FALSE(lasts_to(granted, 2 * ROUNDS_IN_A_TURN));
}

// end of file: ScrollDurationTest.cpp
