// file: CharacterAgeingTest.cpp
// A creature's lifespan reaching its scores, its save, the round upkeep and its death.
//
// - Ability scores read the ageing change on every call and never store it (Lifespan.h).
// - A year passes at the round whose number is a whole number of 365-day years, the
//   owner's calendar, and the character dies of old age when the natural age reaches the
//   maximum, the owner's ruling: "Dies of old age".
// - The race lives in the lifespan and Player::get_race reads it there.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CharacterAgeing*

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/GameLoopCoordinator.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/Lifespan.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A year of the owner's calendar, in rounds, written out: 365 days of 24 hours of 60 rounds.
constexpr int ROUNDS_IN_A_YEAR = 365 * 24 * 60;
constexpr int BASE_SCORE = 16;
} // namespace

class CharacterAgeingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.dice.set_test_mode(false);
		player = std::make_unique<Player>(Vector2D{ 1, 1 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->set_creature_class(CreatureClass::FIGHTER);
		player->set_strength(BASE_SCORE);
		player->set_dexterity(BASE_SCORE);
		player->set_constitution(BASE_SCORE);
		player->set_intelligence(BASE_SCORE);
		player->set_wisdom(BASE_SCORE);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.hungerSystem = &hungerSystem;
		ctx.gameLoopCoordinator = &coordinator;
		ctx.map = &map;
		map.init_tiles();
	}

	// The player as a human of this natural age, living to `maximumAge`.
	void make_human(int naturalAge, int maximumAge)
	{
		player->lifespan =
			Lifespan{ PlayerRaceState::HUMAN, naturalAge, maximumAge, 0, 0 };
	}

	// Runs the upkeep for the one round numbered `round`, as though every round before it had run.
	void run_round(int round)
	{
		ctx.gameState->set_rounds_run(round - 1);
		ctx.gameState->set_time((round - 1) * TIME_UNITS_PER_ROUND);
		ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND);
		coordinator.apply_round_upkeep(ctx);
	}

	std::string all_message_text() const
	{
		std::string joined;
		for (
			size_t index = 0;
			index < ctx.messageSystem->get_stored_message_count();
			++index)
		{
			for (
				const auto& part :
				ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	HungerSystem hungerSystem{};
	GameLoopCoordinator coordinator{};
	Map map{ 20, 20 };
};

TEST_F(CharacterAgeingTest, TheRaceIsReadFromTheLifespan)
{
	player->lifespan->race = PlayerRaceState::DWARF;

	EXPECT_EQ(player->get_race(), PlayerRaceState::DWARF);
	EXPECT_EQ(player->get_race_display_name(), "Dwarf");
}

TEST_F(CharacterAgeingTest, AYoungPlayersScoresAreItsOwn)
{
	make_human(30, 100);

	EXPECT_EQ(player->get_strength(), BASE_SCORE);
	EXPECT_EQ(player->get_wisdom(), BASE_SCORE);
}

// Read on every call: making the character young again gives the scores back unchanged.
TEST_F(CharacterAgeingTest, AgeingIsReadAndNeverWrittenIntoTheScores)
{
	make_human(60, 100);
	ASSERT_EQ(player->get_strength(), BASE_SCORE - 3);
	ASSERT_EQ(player->get_wisdom(), BASE_SCORE + 2);

	make_human(30, 100);

	EXPECT_EQ(player->get_strength(), BASE_SCORE);
	EXPECT_EQ(player->get_dexterity(), BASE_SCORE);
	EXPECT_EQ(player->get_constitution(), BASE_SCORE);
	EXPECT_EQ(player->get_intelligence(), BASE_SCORE);
	EXPECT_EQ(player->get_wisdom(), BASE_SCORE);
}

TEST_F(CharacterAgeingTest, MagicalYearsAgeTheBodyAndNotTheMind)
{
	make_human(30, 100);
	player->lifespan->magicalYears = 15;

	EXPECT_EQ(player->get_strength(), BASE_SCORE - 1);
	EXPECT_EQ(player->get_constitution(), BASE_SCORE - 1);
	EXPECT_EQ(player->get_intelligence(), BASE_SCORE);
	EXPECT_EQ(player->get_wisdom(), BASE_SCORE);
}

TEST_F(CharacterAgeingTest, ExceptionalStrengthAgesBeforeThePoint)
{
	player->set_strength(18);
	player->set_exceptional_strength(50);
	make_human(45, 100);
	EXPECT_EQ(player->get_strength(), 18);
	EXPECT_EQ(player->get_exceptional_strength(), 25);

	make_human(60, 100);
	EXPECT_EQ(player->get_strength(), 17);
	EXPECT_EQ(player->get_exceptional_strength(), 0);
}

TEST_F(CharacterAgeingTest, AMonsterHasNoLifespanAndNoAgeing)
{
	Creature monster{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK },
	};
	monster.set_strength(BASE_SCORE);

	EXPECT_FALSE(monster.lifespan.has_value());
	EXPECT_EQ(monster.get_strength(), BASE_SCORE);
}

TEST_F(CharacterAgeingTest, MagicalYearsAreAddedAndCrossingABandSaysSo)
{
	make_human(44, 100);

	player->add_magical_years(1, ctx);

	EXPECT_EQ(player->lifespan->magicalYears, 1);
	EXPECT_NE(
		all_message_text().find("You feel the years: middle age."),
		std::string::npos)
		<< all_message_text();
}

TEST_F(CharacterAgeingTest, MagicalYearsWithinABandSayNothing)
{
	make_human(30, 100);

	player->add_magical_years(1, ctx);

	EXPECT_EQ(all_message_text().find("You feel the years"), std::string::npos)
		<< all_message_text();
}

TEST_F(CharacterAgeingTest, TheRoundBeforeAYearAddsNone)
{
	make_human(30, 100);

	run_round(ROUNDS_IN_A_YEAR - 1);

	EXPECT_EQ(player->lifespan->yearsLived, 0);
}

TEST_F(CharacterAgeingTest, TheRoundThatEndsAYearAddsOne)
{
	make_human(30, 100);

	run_round(ROUNDS_IN_A_YEAR);

	EXPECT_EQ(player->lifespan->yearsLived, 1);
	EXPECT_EQ(player->lifespan->natural_age(), 31);
}

TEST_F(CharacterAgeingTest, AYearThatReachesMiddleAgeSaysSo)
{
	make_human(44, 100);

	run_round(ROUNDS_IN_A_YEAR);

	EXPECT_NE(
		all_message_text().find("You feel the years: middle age."),
		std::string::npos)
		<< all_message_text();
}

// The owner's ruling: "Dies of old age", at the year that brings the natural age to the maximum.
TEST_F(CharacterAgeingTest, TheYearThatReachesTheMaximumKills)
{
	make_human(92, 93);

	run_round(ROUNDS_IN_A_YEAR);

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::DEFEAT);
	EXPECT_NE(all_message_text().find("You die of old age."), std::string::npos)
		<< all_message_text();
}

TEST_F(CharacterAgeingTest, AYearShortOfTheMaximumLives)
{
	make_human(91, 93);

	run_round(ROUNDS_IN_A_YEAR);

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::UNDECIDED);
}

// Magic ages the body; only time spends the life span.
TEST_F(CharacterAgeingTest, MagicalYearsPastTheMaximumDoNotKill)
{
	make_human(30, 93);
	player->lifespan->magicalYears = 70;

	run_round(ROUNDS_IN_A_YEAR);

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::UNDECIDED);
}

TEST_F(CharacterAgeingTest, ASavedLifespanComesBack)
{
	player->lifespan = Lifespan{ PlayerRaceState::GNOME, 70, 280, 3, 2 };
	nlohmann::json saved;
	player->save(saved);

	auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	loaded->load(saved);

	ASSERT_TRUE(loaded->lifespan.has_value());
	EXPECT_EQ(loaded->get_race(), PlayerRaceState::GNOME);
	EXPECT_EQ(loaded->lifespan->startingAge, 70);
	EXPECT_EQ(loaded->lifespan->maximumAge, 280);
	EXPECT_EQ(loaded->lifespan->yearsLived, 3);
	EXPECT_EQ(loaded->lifespan->magicalYears, 2);
}

TEST_F(CharacterAgeingTest, ALifespanMissingAFieldIsRefused)
{
	nlohmann::json saved;
	player->save(saved);

	for (
		const char* field :
		{ "race", "startingAge", "maximumAge", "yearsLived", "magicalYears" })
	{
		nlohmann::json damaged = saved;
		damaged.at("lifespan").erase(field);
		auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
		EXPECT_ANY_THROW(loaded->load(damaged))
			<< "a lifespan without " << field << " loaded";
	}
}

TEST_F(CharacterAgeingTest, APlayerWithNoLifespanIsRefused)
{
	nlohmann::json saved;
	player->save(saved);
	saved["lifespan"] = nullptr;

	auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	EXPECT_THROW(loaded->load(saved), std::runtime_error);
}

TEST_F(CharacterAgeingTest, AMonstersEmptyLifespanSavesAndLoads)
{
	Creature monster{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK },
	};
	monster.healthPool = std::make_unique<HealthPool>(5);
	monster.armorClass = std::make_unique<ArmorClass>(10);
	nlohmann::json saved;
	monster.save(saved);

	ASSERT_TRUE(saved.contains("lifespan"));
	EXPECT_TRUE(saved.at("lifespan").is_null());

	Creature loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(saved);
	EXPECT_FALSE(loaded.lifespan.has_value());
}

// A new game rolls the character's ages from Table 11 for the race the blueprint chose.
TEST_F(CharacterAgeingTest, ANewGameRollsTheLifespanForTheChosenRace)
{
	PlayerBlueprint blueprint{};
	blueprint.name = "Tester";
	blueprint.creatureClass = CreatureClass::FIGHTER;
	blueprint.playerRaceState = PlayerRaceState::HUMAN;
	blueprint.abilityScores.fill(12);
	player = std::make_unique<Player>(Vector2D{ 1, 1 }, blueprint, ctx);
	ASSERT_EQ(player->get_race(), PlayerRaceState::HUMAN)
		<< "the blueprint's race did not reach the lifespan";
	ASSERT_EQ(player->lifespan->startingAge, 0)
		<< "the ages were rolled before the game started";

	player->on_new_game_start(ctx);

	EXPECT_EQ(player->get_race(), PlayerRaceState::HUMAN);
	EXPECT_GE(player->lifespan->startingAge, 16);
	EXPECT_LE(player->lifespan->startingAge, 19);
	EXPECT_GE(player->lifespan->maximumAge, 92);
	EXPECT_LE(player->lifespan->maximumAge, 130);
}

// end of file: CharacterAgeingTest.cpp
