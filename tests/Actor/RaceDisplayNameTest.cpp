// file: RaceDisplayNameTest.cpp
// A race's name is derived from its enum by race_display_name, and nothing stores it.
// The blueprint, the character and the save all hold the enum alone, so there is no
// name to read back and no parser to disagree with the table.
//
// What is checked here is the table itself, over every race rather than over a sample:
// each race the menu offers has a name, no two share one, and the unchosen state is
// not among the choices.
//
// What it deliberately does not check: what the names are. "Half-Elf" rather than
// "Half Elf" is a presentation choice, and a test asserting the spelling would have to
// be edited to change it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RaceDisplayNameTest.*

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <string_view>

#include "src/Player.h"

// Two races sharing a name would show the player two choices they cannot tell apart.
TEST(RaceDisplayNameTest, NoTwoRacesShareAName)
{
	std::set<std::string> seen;
	for (const PlayerRaceState race : ALL_PLAYER_RACE)
	{
		const std::string name{ race_display_name(race) };
		EXPECT_TRUE(seen.insert(name).second)
			<< "'" << name << "' names more than one race";
	}
}

// A race is never nameless: the menu would offer an empty line.
TEST(RaceDisplayNameTest, NoRaceIsNameless)
{
	for (const PlayerRaceState race : ALL_PLAYER_RACE)
	{
		EXPECT_FALSE(race_display_name(race).empty())
			<< "a race the menu offers has no name to offer";
	}
}

// The state before a race is chosen is not a race, and it prints as what it is.
TEST(RaceDisplayNameTest, TheUnchosenStateNamesItself)
{
	EXPECT_EQ(race_display_name(PlayerRaceState::NONE), "None");
}

// The choices list is what the menu offers and what a random roll picks from, so the
// unchosen state must not be on it. This is the case a dropped entry fails: the list
// is a fixed-size array, so an initializer left out value-initialises its tail to
// NONE, and the name checks above cannot see that because NONE has a name of its own.
TEST(RaceDisplayNameTest, TheUnchosenStateIsNotOneOfTheChoices)
{
	for (const PlayerRaceState race : ALL_PLAYER_RACE)
	{
		EXPECT_NE(race, PlayerRaceState::NONE)
			<< "the race menu offers the unchosen state as though it were a race";
	}
}
