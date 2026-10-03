// file: RaceDisplayNameTest.cpp
// A race's name and its enum cannot disagree, because one of them is computed.
//
// What it is for. MenuRace wrote a race's display name as a string literal and its
// ability modifiers from the enum, separately, at twelve sites; the Player constructor
// then parsed the name back into the enum. A spelling that disagreed left
// playerRaceState at NONE, and every racial adjustment - the six ability modifiers,
// Table 27's thief skill columns, the halfling exception on exceptional Strength -
// silently did nothing. Nothing checked the two agreed, and nothing could: there was no
// single place either fact lived.
//
// What is checked here is the round trip, over every race rather than over a sample,
// so a race added to the enum and left out of one of the two switches fails here
// instead of in play.
//
// What it deliberately does not check: what the names are. "Half-Elf" rather than
// "Half Elf" is a presentation choice, and a test asserting the spelling would have to
// be edited to change it. The one exception is the hyphen, which is asserted as a
// near miss that must not resolve, because that is the class of typo this exists for.
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

// Every race the menu can offer survives being written out and read back.
TEST(RaceDisplayNameTest, EveryRaceSurvivesTheRoundTrip)
{
	for (const Player::PlayerRaceState race : ALL_PLAYER_RACE)
	{
		const std::string_view name = race_display_name(race);
		const std::optional<Player::PlayerRaceState> parsed = race_state_from_display_name(name);

		ASSERT_TRUE(parsed.has_value()) << "no race answers to the name '" << name << "'";
		EXPECT_EQ(*parsed, race) << "'" << name << "' read back as a different race";
	}
}

// Two races sharing a name would make the round trip ambiguous, and the parser would
// answer with whichever branch came first.
TEST(RaceDisplayNameTest, NoTwoRacesShareAName)
{
	std::set<std::string> seen;
	for (const Player::PlayerRaceState race : ALL_PLAYER_RACE)
	{
		const std::string name{ race_display_name(race) };
		EXPECT_TRUE(seen.insert(name).second) << "'" << name << "' names more than one race";
	}
}

// A race is never nameless. An empty name would round-trip through a parser that
// rejected it, so the previous case cannot see this on its own.
TEST(RaceDisplayNameTest, NoRaceIsNameless)
{
	for (const Player::PlayerRaceState race : ALL_PLAYER_RACE)
	{
		EXPECT_FALSE(race_display_name(race).empty()) << "a race the menu offers has no name to offer";
	}
}

// The state before a race is chosen is not a race, and it prints as what it is.
TEST(RaceDisplayNameTest, TheUnchosenStateNamesItself)
{
	EXPECT_EQ(race_display_name(Player::PlayerRaceState::NONE), "None");
}

// The near miss this whole file exists for: a name that is almost right resolves to
// nothing rather than to a race, so a wiring fault cannot pass as a human.
TEST(RaceDisplayNameTest, AMisspeltNameResolvesToNoRace)
{
	EXPECT_FALSE(race_state_from_display_name("Half Elf").has_value()) << "the hyphen in Half-Elf is not optional";
	EXPECT_FALSE(race_state_from_display_name("human").has_value()) << "the parser must not be case-insensitive by accident";
	EXPECT_FALSE(race_state_from_display_name("").has_value());
	EXPECT_FALSE(race_state_from_display_name("Orc").has_value());
}

// The choices list is what the menu offers and what a random roll picks from, so the
// unchosen state must not be on it. This is the case a dropped entry fails: the list
// is a fixed-size array, so an initializer left out value-initialises its tail to
// NONE, and the round trip above cannot see that because NONE has a name of its own.
TEST(RaceDisplayNameTest, TheUnchosenStateIsNotOneOfTheChoices)
{
	for (const Player::PlayerRaceState race : ALL_PLAYER_RACE)
	{
		EXPECT_NE(race, Player::PlayerRaceState::NONE)
			<< "the race menu offers the unchosen state as though it were a race";
	}
}

// A blueprint that never reached the race menu carries NONE's own name, and building
// a character from it is legal - the blueprint says so itself. So the parser answers
// for it rather than refusing, and the constructor's assertion stays about typos.
TEST(RaceDisplayNameTest, TheUnchosenStateReadsBackFromItsName)
{
	const std::optional<Player::PlayerRaceState> parsed = race_state_from_display_name("None");

	ASSERT_TRUE(parsed.has_value()) << "a default blueprint's race name must not read as a wiring fault";
	EXPECT_EQ(*parsed, Player::PlayerRaceState::NONE);
}
