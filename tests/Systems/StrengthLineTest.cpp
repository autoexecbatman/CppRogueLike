// file: StrengthLineTest.cpp
// What the character sheet's Strength line says.
//
// The line reads its hit and damage from AttackStrength::adjustment for a weaponless
// melee swing, which is the same call the attack makes, so the sheet cannot drift from
// what a swing actually does. What nothing checked was the text itself: the sheet is
// drawn with raylib, and a test of the drawing would need a window.
//
// Every expected string here is worked out from Table 1 in src/json/strength.json and
// the line's own format, not copied from a run.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=StrengthLineTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/CharacterSheetUI.h"
#include "src/DataManager.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

class StrengthLineTest : public ::testing::Test
{
protected:
	void SetUp() override { ctx = mock.to_game_context(); }

	// A player at a given Strength, and an exceptional percentile where the score is 18.
	std::unique_ptr<Player> at_strength(int score, int exceptional)
	{
		auto player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->set_strength(score);
		player->set_exceptional_strength(exceptional);
		return player;
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// An ordinary score prints as itself, with the row's hit and damage. Table 1 gives 17
// a +1 to hit and +1 to damage.
TEST_F(StrengthLineTest, AnOrdinaryScorePrintsItsOwnRow)
{
	const auto player = at_strength(17, 0);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR: 17  (+1 hit, +1 dmg)");
}

// Eighteen without a percentile is its own row, and is not the same as 18/01-50.
TEST_F(StrengthLineTest, EighteenWithoutAPercentileIsItsOwnRow)
{
	const auto player = at_strength(18, 0);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR: 18  (+1 hit, +2 dmg)");
}

// Exceptional Strength prints the way the book writes it, and takes the band's row:
// 18/76-90 is +2 to hit and +4 to damage.
TEST_F(StrengthLineTest, ExceptionalStrengthPrintsAsTheBookWritesIt)
{
	const auto player = at_strength(18, 76);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR: 18/76  (+2 hit, +4 dmg)");
}

// A percentile of 100 is written 18/00 rather than 18/100, and is the top row.
TEST_F(StrengthLineTest, APercentileOfAHundredIsWrittenAsZeroZero)
{
	const auto player = at_strength(18, 100);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR: 18/00  (+3 hit, +6 dmg)");
}

// A single-digit score is padded to the width the column reserves, so the bracket that
// follows lines up with every other row of the panel.
TEST_F(StrengthLineTest, ASingleDigitScoreIsPaddedToTheColumnWidth)
{
	const auto player = at_strength(9, 0);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR:  9  (+0 hit, +0 dmg)");
}

// The weakest score the book prints carries penalties rather than bonuses, and the
// signs have to show.
TEST_F(StrengthLineTest, APenaltyShowsItsSign)
{
	const auto player = at_strength(3, 0);

	EXPECT_EQ(CharacterSheetText::strength_line(*player, mock.data_manager), "STR:  3  (-3 hit, -1 dmg)");
}

// end of file: StrengthLineTest.cpp
