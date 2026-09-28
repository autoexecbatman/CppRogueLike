// file: ConstitutionShareOfHitPointsTest.cpp
// Pins the fact the character sheet prints as "Con Bonus".
//
// CharacterSheetUI's combat line computes it as maximum hit points less base hit
// points and labels the difference Constitution. Nothing held that label to
// account, so the next thing that raises maximum hit points without raising base
// - a ring, an item, a spell - would be reported to the player as Constitution
// and nothing would fail.
//
// The difference is the right way to get the figure: a stored running total
// would be a second answer to one question, and it would drift. What was missing
// is the check that keeps the derivation honest, which is what this file is.
//
// The invariant: maximum hit points less base hit points is the accumulated
// Constitution contribution, and nothing else. Level-up maintains it by adding
// the die roll to base and the roll plus the adjustment to the maximum; a
// Constitution change maintains it by moving the maximum alone.
//
// AD&D 2e, Player's Handbook Table 3 (PDF page 33): a Constitution of 16 gives
// +2 hit points per die, for every class - the parenthetical warrior bonus only
// begins at 17, and "all other classes receive maximum bonus of +2 per die".
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ConstitutionShareOfHitPointsTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/LevelUpSystem.h"
#include "src/Player.h"

namespace
{
constexpr int STARTING_HP = 10;
constexpr int FIGHTER_HIT_DIE = 10;
// Table 3: +2 per die at 16, and every class gets it.
constexpr int CONSTITUTION_THAT_GIVES_TWO = 16;
constexpr int BONUS_PER_DIE_AT_SIXTEEN = 2;
// Table 3 prints 0 from 7 through 14.
constexpr int CONSTITUTION_THAT_GIVES_NOTHING = 10;
constexpr int HIT_DIE_ROLL = 6;
} // namespace

class ConstitutionShareOfHitPointsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		// Base is left to the pool's own constructor, which is what sets it in
		// production - Player never calls set_hp_base. A fixture that set it here
		// would be establishing the very thing the first test asks about.
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->set_dr(0);
		player->set_thaco(0);
		player->set_creature_class(CreatureClass::FIGHTER);
		player->set_hit_die(FIGHTER_HIT_DIE);

		ctx = game.context();
		ctx.playerOwner = &player;

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// What CharacterSheetUI prints as "Con Bonus".
	int constitution_share() const
	{
		return player->get_max_hp() - player->get_hp_base();
	}

	// One level, with the hit die scripted so the roll is not what varies.
	void gain_a_level(int newLevel)
	{
		game.dice.set_next_d20(HIT_DIE_ROLL);
		LevelUpSystem::apply_level_up_benefits(*player, newLevel, &ctx);
	}

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player;
};

// A body as built owes nothing to Constitution: the pool starts with its base
// equal to its maximum.
TEST_F(ConstitutionShareOfHitPointsTest, AFreshCharacterOwesNothingToConstitution)
{
	EXPECT_EQ(constitution_share(), 0);
}

// The level-up adds the roll to base and the roll plus the adjustment to the
// maximum, so the share grows by the adjustment and by nothing else.
TEST_F(ConstitutionShareOfHitPointsTest, EachLevelAddsOnlyItsAdjustmentToTheShare)
{
	player->set_constitution(CONSTITUTION_THAT_GIVES_TWO);

	gain_a_level(2);
	EXPECT_EQ(constitution_share(), BONUS_PER_DIE_AT_SIXTEEN) << "one level's worth";
	EXPECT_EQ(player->get_hp_base(), STARTING_HP + HIT_DIE_ROLL) << "base carries the die alone";

	gain_a_level(3);
	EXPECT_EQ(constitution_share(), BONUS_PER_DIE_AT_SIXTEEN * 2) << "two levels' worth";
	EXPECT_EQ(player->get_hp_base(), STARTING_HP + HIT_DIE_ROLL * 2);
}

// The case that catches base drifting: at a score the table gives nothing, the
// share has to stay at zero however many levels are gained. A level-up that
// stopped feeding base would show up here as hit points attributed to a
// Constitution that grants none.
TEST_F(ConstitutionShareOfHitPointsTest, AScoreTheTableGivesNothingLeavesTheShareEmpty)
{
	player->set_constitution(CONSTITUTION_THAT_GIVES_NOTHING);

	gain_a_level(2);
	gain_a_level(3);

	EXPECT_EQ(constitution_share(), 0) << "hit points reported as Constitution that it never granted";
	EXPECT_EQ(player->get_max_hp(), player->get_hp_base());
}

// end of file: ConstitutionShareOfHitPointsTest.cpp
