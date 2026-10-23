// file: StartingHitPointsTest.cpp
// A new character's hit points before Constitution: the owner's cushion of 20 plus
// one roll of the class's own hit die - "20 + class HD roll", an exception to the
// book recorded 2026-09-19. The dice are the Player's Handbook's: a warrior's d10,
// a priest's d8, a rogue's d6, a wizard's d4.
//
// Real dice, seeded: across two hundred characters of a class every result lies in
// 21 through 20 plus the die, and both ends turn up - so a wrong die, a maximum in
// place of a roll, or a lost cushion each fall outside the range.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=StartingHitPointsTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>

#include "src/CreatureClass.h"
#include "src/GameContext.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int CUSHION = 20;
constexpr unsigned int CHARACTERS_PER_CLASS = 200;

} // namespace

class StartingHitPointsTest : public ::testing::Test
{
protected:
	// The class a character built from a blueprint of this class turns out to hold.
	CreatureClass built_class(CreatureClass creatureClass)
	{
		RandomDice seededDice{ 1 };
		GameContext ctx = mock.to_game_context();
		ctx.dice = &seededDice;

		PlayerBlueprint blueprint{};
		blueprint.name = "Tester";
		blueprint.creatureClass = creatureClass;
		blueprint.playerRaceState = PlayerRaceState::HUMAN;
		const Player player{ Vector2D{ 0, 0 }, blueprint, ctx };
		return player.get_creature_class();
	}

	// The hit points a character of this class starts with, rolled on dice seeded `seed`.
	int starting_hit_points(CreatureClass creatureClass, unsigned int seed)
	{
		RandomDice seededDice{ seed };
		GameContext ctx = mock.to_game_context();
		ctx.dice = &seededDice;

		PlayerBlueprint blueprint{};
		blueprint.name = "Tester";
		blueprint.creatureClass = creatureClass;
		blueprint.playerRaceState = PlayerRaceState::HUMAN;
		const Player player{ Vector2D{ 0, 0 }, blueprint, ctx };
		return player.get_max_hp();
	}

	MockGameContext mock{};
};

// The class the menu chose is the class the character holds: the blueprint's enum is
// the only record of it, so nothing else could supply it.
TEST_F(StartingHitPointsTest, EachClassIsBuiltAsTheClassChosen)
{
	for (const CreatureClass creatureClass : ALL_PLAYER_CLASS)
	{
		EXPECT_EQ(built_class(creatureClass), creatureClass)
			<< encode_creature_class(creatureClass);
	}
}

TEST_F(StartingHitPointsTest, EachClassRollsItsOwnDieOnTopOfTwenty)
{
	struct ClassDie
	{
		CreatureClass creatureClass{ CreatureClass::MONSTER };
		int die{ 0 };
	};

	constexpr std::array<ClassDie, 4> FROM_THE_CLASS_TABLES{ {
		{ CreatureClass::FIGHTER, 10 },
		{ CreatureClass::CLERIC, 8 },
		{ CreatureClass::ROGUE, 6 },
		{ CreatureClass::WIZARD, 4 },
	} };

	for (const ClassDie& expected : FROM_THE_CLASS_TABLES)
	{
		int lowest = CUSHION + expected.die + 1;
		int highest = 0;
		for (unsigned int seed = 1; seed <= CHARACTERS_PER_CLASS; ++seed)
		{
			const int hitPoints =
				starting_hit_points(expected.creatureClass, seed);
			lowest = std::min(lowest, hitPoints);
			highest = std::max(highest, hitPoints);
		}

		EXPECT_EQ(lowest, CUSHION + 1)
			<< encode_creature_class(expected.creatureClass);
		EXPECT_EQ(highest, CUSHION + expected.die)
			<< encode_creature_class(expected.creatureClass);
	}
}
