// file: TrapSerializationTest.cpp
// What a trap has to carry across a save, and what it must not.
//
// Traps were absent from the save file entirely. A level reloaded in a fresh process
// came back with none of them; one reloaded mid-session kept the abandoned level's
// traps, at their old coordinates, on top of the new map. The one-attempt-per-level
// rule was defeated by the same gap, which is what issues/0037 named.
//
// The claim these cases make is that a trap stores only what the constructor cannot
// rebuild - position, type, state, and the levels its last disarm attempt and its last
// search were made at - and that everything else comes back from the type table rather
// than from the save.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapSerializationTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/Actor.h"
#include "src/Creature.h"
#include "src/GameStateManager.h"
#include "src/HealthPool.h"
#include "src/Persistent.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/ThiefSkills.h"
#include "src/Trap.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class TrapSerializationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		finder.healthPool = std::make_unique<HealthPool>(20);
		// A dwarf thief can find every kind of trap. Dexterity 13 leaves its Find/Remove
		// Traps at the base, the race and the points.
		finder.set_creature_class(CreatureClass::ROGUE);
		finder.playerClassState = Player::PlayerClassState::ROGUE;
		finder.set_dexterity(13);
		finder.playerRaceState = PlayerRaceState::DWARF;
		finder.set_creature_level(1);
		finder.thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = 40;
	}

	std::unique_ptr<Trap> built(Vector2D position, TrapType type)
	{
		return std::make_unique<Trap>(position, type, *ctx.tileConfig);
	}

	// Finds the trap the way the game does, rather than through a test-only setter: a
	// search rolling 1, which finds a pit on 1d6 and a small trap on d100.
	void find(Trap& trap)
	{
		mock.dice.set_next_roll(1);
		ASSERT_EQ(trap.search(finder, ctx), TrapSearchResult::FOUND);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Player finder{ Vector2D{ 0, 0 } };
};

// The gap itself: every trap on the level comes back, at the tile it was on.
TEST_F(TrapSerializationTest, EveryTrapComesBackWhereItWas)
{
	std::vector<std::unique_ptr<Trap>> traps;
	traps.push_back(built(Vector2D{ 4, 3 }, TrapType::PIT));
	traps.push_back(built(Vector2D{ 9, 7 }, TrapType::DART));

	json level;
	save_traps(traps, level);

	std::vector<std::unique_ptr<Trap>> restored;
	load_traps(level, restored, *ctx.tileConfig);

	ASSERT_EQ(restored.size(), 2u) << "the level came back with a different number of traps";
	EXPECT_EQ(restored.at(0)->position, (Vector2D{ 4, 3 }));
	EXPECT_EQ(restored.at(1)->position, (Vector2D{ 9, 7 }));
	EXPECT_EQ(restored.at(0)->get_type(), TrapType::PIT);
	EXPECT_EQ(restored.at(1)->get_type(), TrapType::DART);
}

// What the player has learned survives: a trap they have spotted is still spotted, and
// one they have not is still hidden. Reloading must not re-hide a found trap.
TEST_F(TrapSerializationTest, WhatThePlayerHasFoundStaysFound)
{
	std::vector<std::unique_ptr<Trap>> traps;
	traps.push_back(built(Vector2D{ 2, 2 }, TrapType::ARROW));
	traps.push_back(built(Vector2D{ 5, 5 }, TrapType::PIT));
	find(*traps.at(0));

	json level;
	save_traps(traps, level);

	std::vector<std::unique_ptr<Trap>> restored;
	load_traps(level, restored, *ctx.tileConfig);

	ASSERT_EQ(restored.size(), 2u);
	EXPECT_EQ(restored.at(0)->get_state(), TrapState::DETECTED) << "a found trap was hidden again";
	EXPECT_EQ(restored.at(1)->get_state(), TrapState::HIDDEN) << "an unfound trap was revealed";
}

// Visibility follows from the state rather than being stored, so a restored trap is
// invisible exactly when it is still hidden. Storing it would be a second answer to a
// question the state already settles.
TEST_F(TrapSerializationTest, VisibilityIsDerivedFromTheStateOnLoad)
{
	std::vector<std::unique_ptr<Trap>> traps;
	traps.push_back(built(Vector2D{ 2, 2 }, TrapType::ARROW));
	traps.push_back(built(Vector2D{ 5, 5 }, TrapType::PIT));
	find(*traps.at(0));

	json level;
	save_traps(traps, level);

	std::vector<std::unique_ptr<Trap>> restored;
	load_traps(level, restored, *ctx.tileConfig);

	ASSERT_EQ(restored.size(), 2u);
	EXPECT_FALSE(restored.at(0)->has_state(ActorState::IS_INVISIBLE)) << "a found trap came back invisible";
	EXPECT_TRUE(restored.at(1)->has_state(ActorState::IS_INVISIBLE)) << "a hidden trap came back visible";
}

// A search missed at this level is not handed back by reloading: the trap remembers the
// level it was searched at, and a certain find after the load must not be reached.
TEST_F(TrapSerializationTest, AMissedSearchIsNotHandedBackByReloading)
{
	std::vector<std::unique_ptr<Trap>> traps;
	traps.push_back(built(Vector2D{ 2, 2 }, TrapType::DART));
	mock.dice.set_next_roll(100);
	ASSERT_EQ(traps.at(0)->search(finder, ctx), TrapSearchResult::MISSED);

	json level;
	save_traps(traps, level);
	std::vector<std::unique_ptr<Trap>> restored;
	load_traps(level, restored, *ctx.tileConfig);

	ASSERT_EQ(restored.size(), 1u);
	mock.dice.set_next_roll(1);
	EXPECT_EQ(restored.at(0)->search(finder, ctx), TrapSearchResult::ALREADY_SEARCHED);
	EXPECT_EQ(restored.at(0)->get_state(), TrapState::HIDDEN);
}

// A mid-session load must not leave the abandoned level's traps on the new map, which
// is the half of the gap a round trip on an empty container cannot see.
TEST_F(TrapSerializationTest, LoadingReplacesTheTrapsAlreadyThere)
{
	json emptyLevel;
	save_traps({}, emptyLevel);

	std::vector<std::unique_ptr<Trap>> stale;
	stale.push_back(built(Vector2D{ 1, 1 }, TrapType::PIT));

	load_traps(emptyLevel, stale, *ctx.tileConfig);

	EXPECT_TRUE(stale.empty()) << "the previous level's traps survived the load";
}

// end of file: TrapSerializationTest.cpp
