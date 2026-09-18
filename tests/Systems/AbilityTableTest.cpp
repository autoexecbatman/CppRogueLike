// Checks the Strength and Constitution tables against the Player's Handbook, exceptional
// Strength included, and that the game reads them wherever it uses them.
//
// What it is for. Table 1 (Strength, PDF pages 30-31 of the 2e archive) gives a warrior
// with Strength 18 a percentile roll, 18/01 to 18/00, and five bands of it above plain 18;
// Table 3 (Constitution, pages 33-34) runs to 25. Both tables stopped short: Strength had
// no 18/xx, Constitution ended at 19 though a dwarf with an amulet of health reaches 20,
// and a creature past the end of either got nothing - or, attacking past Strength 25,
// never swung at all. The rows below were generated from the book's text, not typed:
// Strength's parenthesised door chances are dropped, as the data drops them, and
// Constitution's hit point column is the warrior's, with regeneration as the turns per
// point, 0 for none.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AbilityTableTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <vector>

#include "src/ArmorClass.h"
#include "src/AttackKind.h"
#include "src/ConstitutionAttributes.h"
#include "src/ConstitutionTracker.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/StrengthAttributes.h"

namespace
{
// One plain row of Table 1: the score, hit, damage, weight allowance and maximum press.
struct StrengthRow
{
	int score{ 0 };
	int hit{ 0 };
	int damage{ 0 };
	int weightAllowance{ 0 };
	int maximumPress{ 0 };
};

// One exceptional band of Table 1: its percentile range and the same four columns.
struct StrengthBand
{
	int from{ 0 };
	int to{ 0 };
	int hit{ 0 };
	int damage{ 0 };
	int weightAllowance{ 0 };
	int maximumPress{ 0 };
};

// One row of Table 3.
struct ConstitutionRow
{
	int score{ 0 };
	int hitPoints{ 0 };
	int systemShock{ 0 };
	int resurrection{ 0 };
	int poisonSave{ 0 };
	int regenerationTurns{ 0 };
};

const std::vector<StrengthRow> TABLE_ONE{
	{ 1, -5, -4, 1, 3 },
	{ 2, -3, -2, 1, 5 },
	{ 3, -3, -1, 5, 10 },
	{ 4, -2, -1, 10, 25 },
	{ 5, -2, -1, 10, 25 },
	{ 6, -1, 0, 20, 55 },
	{ 7, -1, 0, 20, 55 },
	{ 8, 0, 0, 35, 90 },
	{ 9, 0, 0, 35, 90 },
	{ 10, 0, 0, 40, 115 },
	{ 11, 0, 0, 40, 115 },
	{ 12, 0, 0, 45, 140 },
	{ 13, 0, 0, 45, 140 },
	{ 14, 0, 0, 55, 170 },
	{ 15, 0, 0, 55, 170 },
	{ 16, 0, 1, 70, 195 },
	{ 17, 1, 1, 85, 220 },
	{ 18, 1, 2, 110, 255 },
	{ 19, 3, 7, 485, 640 },
	{ 20, 3, 8, 535, 700 },
	{ 21, 4, 9, 635, 810 },
	{ 22, 4, 10, 785, 970 },
	{ 23, 5, 11, 935, 1130 },
	{ 24, 6, 12, 1235, 1440 },
	{ 25, 7, 14, 1535, 1750 },
};

const std::vector<StrengthBand> EXCEPTIONAL_BANDS{
	{ 1, 50, 1, 3, 135, 280 },
	{ 51, 75, 2, 3, 160, 305 },
	{ 76, 90, 2, 4, 185, 330 },
	{ 91, 99, 2, 5, 235, 380 },
	{ 100, 100, 3, 6, 335, 480 },
};

const std::vector<ConstitutionRow> TABLE_THREE{
	{ 1, -3, 25, 30, -2, 0 },
	{ 2, -2, 30, 35, -1, 0 },
	{ 3, -2, 35, 40, 0, 0 },
	{ 4, -1, 40, 45, 0, 0 },
	{ 5, -1, 45, 50, 0, 0 },
	{ 6, -1, 50, 55, 0, 0 },
	{ 7, 0, 55, 60, 0, 0 },
	{ 8, 0, 60, 65, 0, 0 },
	{ 9, 0, 65, 70, 0, 0 },
	{ 10, 0, 70, 75, 0, 0 },
	{ 11, 0, 75, 80, 0, 0 },
	{ 12, 0, 80, 85, 0, 0 },
	{ 13, 0, 85, 90, 0, 0 },
	{ 14, 0, 88, 92, 0, 0 },
	{ 15, 1, 90, 94, 0, 0 },
	{ 16, 2, 95, 96, 0, 0 },
	{ 17, 3, 97, 98, 0, 0 },
	{ 18, 4, 99, 100, 0, 0 },
	{ 19, 5, 99, 100, 1, 0 },
	{ 20, 5, 99, 100, 1, 6 },
	{ 21, 6, 99, 100, 2, 5 },
	{ 22, 6, 99, 100, 2, 4 },
	{ 23, 6, 99, 100, 3, 3 },
	{ 24, 7, 99, 100, 3, 2 },
	{ 25, 7, 100, 100, 4, 1 },
};
} // namespace

class AbilityTableTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		ctx = game.context();
		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// A 1d4 attacker from THAC0 20 with the given Strength, and a target at armour class 10.
	std::unique_ptr<Creature> brute_with(int strength, int exceptional)
	{
		auto brute = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "brute", 0 });
		brute->experienceReward = std::make_unique<ExperienceReward>(0);
		brute->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		brute->armorClass = std::make_unique<ArmorClass>(10);
		brute->attacker = std::make_unique<MonsterAttacker>(*brute, DamageInfo{ "1d4", DamageType::PHYSICAL });
		brute->set_thaco(20);
		brute->set_dr(0);
		brute->set_dexterity(10);
		brute->set_strength(strength);
		brute->set_exceptional_strength(exceptional);
		brute->set_natural_attack("fists");
		return brute;
	}

	// Queues rolls in the order the game asks for them.
	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			game.dice.set_next_roll(roll);
		}
	}

	static constexpr int STARTING_HP = 100;

	Game game;
	GameContext ctx;
};

// Every plain row of Table 1, 1 to 25.
TEST_F(AbilityTableTest, StrengthRowsMatchTableOne)
{
	for (const StrengthRow& expected : TABLE_ONE)
	{
		const StrengthAttributes row = game.dataManager.strength_for(expected.score, 0);
		EXPECT_EQ(row.hitProb, expected.hit) << "hit at Strength " << expected.score;
		EXPECT_EQ(row.dmgAdj, expected.damage) << "damage at Strength " << expected.score;
		EXPECT_EQ(row.wgtAllow, expected.weightAllowance) << "weight allowance at Strength " << expected.score;
		EXPECT_EQ(row.maxPress, expected.maximumPress) << "maximum press at Strength " << expected.score;
	}
}

// Every 18/xx band, read at both ends of its percentile range.
TEST_F(AbilityTableTest, ExceptionalBandsMatchTableOne)
{
	for (const StrengthBand& expected : EXCEPTIONAL_BANDS)
	{
		for (const int percentile : { expected.from, expected.to })
		{
			const StrengthAttributes row = game.dataManager.strength_for(18, percentile);
			EXPECT_EQ(row.hitProb, expected.hit) << "hit at 18/" << percentile;
			EXPECT_EQ(row.dmgAdj, expected.damage) << "damage at 18/" << percentile;
			EXPECT_EQ(row.wgtAllow, expected.weightAllowance) << "weight allowance at 18/" << percentile;
			EXPECT_EQ(row.maxPress, expected.maximumPress) << "maximum press at 18/" << percentile;
		}
	}
}

// The book gives exceptional Strength only to an 18: a 17 or a 19 ignores the percentile.
TEST_F(AbilityTableTest, ExceptionalStrengthCountsOnlyAtEighteen)
{
	EXPECT_EQ(game.dataManager.strength_for(17, 100).dmgAdj, 1);
	EXPECT_EQ(game.dataManager.strength_for(19, 100).dmgAdj, 7);
}

// Every row of Table 3, 1 to 25.
TEST_F(AbilityTableTest, ConstitutionRowsMatchTableThree)
{
	for (const ConstitutionRow& expected : TABLE_THREE)
	{
		const ConstitutionAttributes row = game.dataManager.constitution_for(expected.score);
		EXPECT_EQ(row.HPAdj, expected.hitPoints) << "hit points at Constitution " << expected.score;
		EXPECT_EQ(row.SystemShock, expected.systemShock) << "system shock at Constitution " << expected.score;
		EXPECT_EQ(row.ResurrectionSurvival, expected.resurrection) << "resurrection at Constitution " << expected.score;
		EXPECT_EQ(row.PoisonSave, expected.poisonSave) << "poison save at Constitution " << expected.score;
		EXPECT_EQ(row.Regeneration, expected.regenerationTurns) << "regeneration at Constitution " << expected.score;
	}
}

// Past 25 each table reads its last row; below 1, no adjustment.
TEST_F(AbilityTableTest, BothTablesHaveTheDexterityTablesEdges)
{
	EXPECT_EQ(game.dataManager.strength_for(26, 0).dmgAdj, 14);
	EXPECT_EQ(game.dataManager.constitution_for(26).HPAdj, 7);
	EXPECT_EQ(game.dataManager.strength_for(0, 0).dmgAdj, 0);
	EXPECT_EQ(game.dataManager.constitution_for(0).HPAdj, 0);
}

// 18/00 is +3 to hit and +6 damage: THAC0 20 against armour class 10 needs 10, and the
// 7 rolled hits only at +3; the die's 4 lands as 10.
TEST_F(AbilityTableTest, EighteenHundredStrikesWithTheBandsBonuses)
{
	auto brute = brute_with(18, 100);
	auto target = brute_with(10, 0);
	script({ 7, 4 });

	brute->attacker->attack(*target, AttackKind::MELEE, ctx);

	EXPECT_EQ(STARTING_HP - target->get_hp(), 10) << "18/00 hit and damage";
}

// Past the table an attacker still swings: Strength 26 reads 25, +14 damage on the die's 4.
TEST_F(AbilityTableTest, StrengthPastTheTableStillAttacks)
{
	auto brute = brute_with(26, 0);
	auto target = brute_with(10, 0);
	script({ 20, 4 });

	brute->attacker->attack(*target, AttackKind::MELEE, ctx);

	EXPECT_EQ(STARTING_HP - target->get_hp(), 18) << "an attacker past Strength 25 never swung";
}

// A warrior's hit point bonus at Constitution 20 is +5, where the table used to end at 19.
TEST_F(AbilityTableTest, ConstitutionTwentyKeepsAWarriorsHitPointBonus)
{
	auto dwarf = brute_with(10, 0);
	dwarf->set_creature_class(CreatureClass::FIGHTER);
	dwarf->set_constitution(20);
	ConstitutionTracker tracker{};

	EXPECT_EQ(tracker.apply_constitution_changes(*dwarf, ctx).newBonus, 5);
}

// Character creation: a human fighter with Strength 18 rolls d100 - here 76.
TEST_F(AbilityTableTest, AFighterWithEighteenRollsExceptionalStrength)
{
	Player fighter{ Vector2D{ 0, 0 } };
	fighter.set_creature_class(CreatureClass::FIGHTER);
	fighter.playerRaceState = Player::PlayerRaceState::HUMAN;
	fighter.set_strength(18);
	script({ 76 });

	fighter.roll_exceptional_strength(ctx);

	EXPECT_EQ(fighter.get_exceptional_strength(), 76);
}

// "Halfling fighters do not roll for exceptional Strength", nor does anyone but a
// warrior, nor a warrior short of 18.
TEST_F(AbilityTableTest, OnlyANonHalflingFighterWithEighteenRolls)
{
	Player halfling{ Vector2D{ 0, 0 } };
	halfling.set_creature_class(CreatureClass::FIGHTER);
	halfling.playerRaceState = Player::PlayerRaceState::HALFLING;
	halfling.set_strength(18);

	Player cleric{ Vector2D{ 0, 0 } };
	cleric.set_creature_class(CreatureClass::CLERIC);
	cleric.playerRaceState = Player::PlayerRaceState::HUMAN;
	cleric.set_strength(18);

	Player weaker{ Vector2D{ 0, 0 } };
	weaker.set_creature_class(CreatureClass::FIGHTER);
	weaker.playerRaceState = Player::PlayerRaceState::HUMAN;
	weaker.set_strength(17);

	script({ 76, 76, 76 });
	halfling.roll_exceptional_strength(ctx);
	cleric.roll_exceptional_strength(ctx);
	weaker.roll_exceptional_strength(ctx);

	EXPECT_EQ(halfling.get_exceptional_strength(), 0);
	EXPECT_EQ(cleric.get_exceptional_strength(), 0);
	EXPECT_EQ(weaker.get_exceptional_strength(), 0);
}
