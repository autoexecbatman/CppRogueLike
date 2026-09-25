// file: EncumbranceBandTest.cpp
// Player's Handbook Table 47, Character Encumbrance (PDF page 159), against the
// Strength rows the game loads.
//
// The defect this pins: WeightTier split the carrying maximum into thirds and called
// the pieces Light, Moderate and Heavy. The book prints five named bands per Strength
// row and they are not a fraction of anything - above its allowance Strength 10-11
// splits 18/18/20 where Strength 18 splits 39/39/39 - so a formula cannot produce
// them and the invented thirds were wrong on every row.
//
// The book prints no row for Strength 1 and none above 18/00, so those scores have no
// band at all rather than one this game made up.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EncumbranceBandTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/DataManager.h"
#include "src/Encumbrance.h"
#include "src/StrengthAttributes.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// One printed row of Table 47: the score that reads it, the four boundaries and the
// Max. Carried Weight the Severe band runs up to.
struct TableFortySevenRow
{
	int strength{ 0 };
	int exceptional{ 0 };
	int unencumberedTo{ 0 };
	int lightTo{ 0 };
	int moderateTo{ 0 };
	int heavyTo{ 0 };
	int maxCarried{ 0 };
};

// Every row the table prints, with the book's ranges expanded to the scores that read
// them: the "4-5" row is checked at 4 and at 5.
constexpr std::array<TableFortySevenRow, 21> TABLE_FORTY_SEVEN{ {
	{ 2, 0, 1, 2, 3, 4, 6 },
	{ 3, 0, 5, 6, 7, 9, 10 },
	{ 4, 0, 10, 13, 16, 19, 25 },
	{ 5, 0, 10, 13, 16, 19, 25 },
	{ 6, 0, 20, 29, 38, 46, 55 },
	{ 7, 0, 20, 29, 38, 46, 55 },
	{ 8, 0, 35, 50, 65, 80, 90 },
	{ 9, 0, 35, 50, 65, 80, 90 },
	{ 10, 0, 40, 58, 76, 96, 110 },
	{ 11, 0, 40, 58, 76, 96, 110 },
	{ 12, 0, 45, 69, 93, 117, 140 },
	{ 13, 0, 45, 69, 93, 117, 140 },
	{ 14, 0, 55, 85, 115, 145, 170 },
	{ 15, 0, 55, 85, 115, 145, 170 },
	{ 16, 0, 70, 100, 130, 160, 195 },
	{ 17, 0, 85, 121, 157, 193, 220 },
	{ 18, 0, 110, 149, 188, 227, 255 },
	{ 18, 25, 135, 174, 213, 252, 280 },
	{ 18, 60, 160, 199, 238, 277, 305 },
	{ 18, 80, 185, 224, 263, 302, 330 },
	{ 18, 95, 235, 274, 313, 352, 380 },
} };

// The last exceptional band, 18/00, which the game stores as a percentile of 100.
constexpr TableFortySevenRow EIGHTEEN_HUNDRED{ 18, 100, 335, 374, 413, 452, 480 };
} // namespace

class EncumbranceBandTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
};

// The four boundaries of every row, read off the table rather than computed.
TEST_F(EncumbranceBandTest, EveryRowTheBookPrintsCarriesItsFourBoundaries)
{
	const auto check = [this](const TableFortySevenRow& expected)
	{
		const StrengthAttributes row = mock.data_manager.strength_for(expected.strength, expected.exceptional);
		ASSERT_TRUE(row.encumbrance.has_value()) << "Strength " << expected.strength << "/" << expected.exceptional;
		EXPECT_EQ(row.encumbrance->unencumberedTo, expected.unencumberedTo) << expected.strength;
		EXPECT_EQ(row.encumbrance->lightTo, expected.lightTo) << expected.strength;
		EXPECT_EQ(row.encumbrance->moderateTo, expected.moderateTo) << expected.strength;
		EXPECT_EQ(row.encumbrance->heavyTo, expected.heavyTo) << expected.strength;
		EXPECT_EQ(row.maxCarried, expected.maxCarried) << expected.strength;
	};

	for (const TableFortySevenRow& expected : TABLE_FORTY_SEVEN)
	{
		check(expected);
	}
	check(EIGHTEEN_HUNDRED);
}

// Each boundary is the last pound its band holds, and one more moves to the next.
TEST_F(EncumbranceBandTest, EachBoundaryIsTheLastPoundOfItsBand)
{
	const StrengthAttributes row = mock.data_manager.strength_for(10, 0);

	EXPECT_EQ(encumbrance_band(0, row), EncumbranceBand::UNENCUMBERED);
	EXPECT_EQ(encumbrance_band(40, row), EncumbranceBand::UNENCUMBERED);
	EXPECT_EQ(encumbrance_band(41, row), EncumbranceBand::LIGHT);
	EXPECT_EQ(encumbrance_band(58, row), EncumbranceBand::LIGHT);
	EXPECT_EQ(encumbrance_band(59, row), EncumbranceBand::MODERATE);
	EXPECT_EQ(encumbrance_band(76, row), EncumbranceBand::MODERATE);
	EXPECT_EQ(encumbrance_band(77, row), EncumbranceBand::HEAVY);
	EXPECT_EQ(encumbrance_band(96, row), EncumbranceBand::HEAVY);
	EXPECT_EQ(encumbrance_band(97, row), EncumbranceBand::SEVERE);
}

// "The Max. Carried Wgt. column lists the most weight your character can carry and
// still move", so the maximum is the last pound of Severe and the next is past it.
TEST_F(EncumbranceBandTest, TheMaximumIsTheLastPoundOfSevere)
{
	const StrengthAttributes row = mock.data_manager.strength_for(10, 0);

	EXPECT_EQ(encumbrance_band(110, row), EncumbranceBand::SEVERE);
	EXPECT_EQ(encumbrance_band(111, row), EncumbranceBand::OVERLOADED);
	EXPECT_EQ(encumbrance_band(1000, row), EncumbranceBand::OVERLOADED);
}

// Exceptional Strength reads its own row: an 18/00 fighter is unencumbered under a
// load that would be Severe for a plain 18.
TEST_F(EncumbranceBandTest, ExceptionalStrengthReadsItsOwnBands)
{
	const StrengthAttributes plain = mock.data_manager.strength_for(18, 0);
	const StrengthAttributes exceptional = mock.data_manager.strength_for(18, 100);

	EXPECT_EQ(encumbrance_band(240, plain), EncumbranceBand::SEVERE);
	EXPECT_EQ(encumbrance_band(240, exceptional), EncumbranceBand::UNENCUMBERED);
}

// Table 47 starts at Strength 2 and stops at 18/00. A score outside that has no band,
// which is the book's silence rather than a band the game invented.
TEST_F(EncumbranceBandTest, AScoreTheTableDoesNotPrintHasNoBand)
{
	for (const int score : { 1, 19, 20, 21, 22, 23, 24, 25 })
	{
		const StrengthAttributes row = mock.data_manager.strength_for(score, 0);
		EXPECT_FALSE(row.encumbrance.has_value()) << "Strength " << score;
		EXPECT_FALSE(encumbrance_band(10, row).has_value()) << "Strength " << score;
	}
}

// The names the panel prints are the book's own words.
TEST_F(EncumbranceBandTest, TheNamesAreTheOnesTheTablePrints)
{
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::UNENCUMBERED), "Unencumbered");
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::LIGHT), "Light");
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::MODERATE), "Moderate");
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::HEAVY), "Heavy");
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::SEVERE), "Severe");
	EXPECT_EQ(encumbrance_band_name(EncumbranceBand::OVERLOADED), "Overloaded");
}

// The bands rise, so no load can fall in two of them and none is unreachable.
TEST_F(EncumbranceBandTest, EveryRowsBoundariesRiseToItsMaximum)
{
	for (const TableFortySevenRow& expected : TABLE_FORTY_SEVEN)
	{
		const StrengthAttributes row = mock.data_manager.strength_for(expected.strength, expected.exceptional);
		ASSERT_TRUE(row.encumbrance.has_value());
		EXPECT_LT(row.encumbrance->unencumberedTo, row.encumbrance->lightTo) << expected.strength;
		EXPECT_LT(row.encumbrance->lightTo, row.encumbrance->moderateTo) << expected.strength;
		EXPECT_LT(row.encumbrance->moderateTo, row.encumbrance->heavyTo) << expected.strength;
		EXPECT_LT(row.encumbrance->heavyTo, row.maxCarried) << expected.strength;
	}
}
