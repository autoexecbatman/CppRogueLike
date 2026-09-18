// file: ConstitutionTrackerTest.cpp
// A creature's constitution bonus to hit points is applied once when the
// creature exists, and reported as a change only when the score actually moves
// between two values the creature has held.
//
// Expected values come from src/json/constitution.json, the table the game
// loads: Con 17 carries HPAdj +3 and Con 18 carries +4. A new character rolls
// 20 + d10 base hit points at level 1, so with the die forced to 5 the base is
// 25, the bonus makes it 28, and a point of Constitution adds one more.
//
// The defect this pins: the tracker's remembered score started at 0, a value no
// creature ever had, so the first tick of every new character logged
// "Constitution increased from 0 to 17!" - a change report with only one state.

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/Player.h"
#include "src/GameContext.h"
#include "src/DataManager.h"
#include "src/MessageSystem.h"
#include "tests/mocks/MockGameContext.h"

class ConstitutionTrackerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		dataManager.load_all_data(mock.messages);
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;

		// Six ability scores, three d6 each, in the order the character rolls
		// them: Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma.
		// Constitution is forced to 17; every other score is an unremarkable 9.
		for (int die = 0; die < 6; ++die)
		{
			mock.dice.set_next_roll(3);
		}
		mock.dice.set_next_roll(6);
		mock.dice.set_next_roll(6);
		mock.dice.set_next_roll(5);
		for (int die = 0; die < 9; ++die)
		{
			mock.dice.set_next_roll(3);
		}
		// Then the d10 of starting hit points.
		mock.dice.set_next_roll(5);

	}

	// Rolls a character of the given class with the forced scores above. The
	// class decides the cap: warriors keep the whole table, every other class
	// stops at +2 (Player's Handbook, Table 3).
	void make_player(std::string_view playerClass)
	{
		PlayerBlueprint blueprint{};
		blueprint.name = "Tester";
		blueprint.playerClass = std::string{ playerClass };
		blueprint.playerRace = "Human";
		player = std::make_unique<Player>(Vector2D{ 0, 0 }, blueprint, ctx);
		ctx.playerOwner = &player;
	}

	// Every message the player would have seen that mentions Constitution.
	std::vector<std::string> constitution_messages() const
	{
		std::vector<std::string> found;
		for (size_t index = 0; index < mock.messages.get_stored_message_count(); ++index)
		{
			for (const auto& part : mock.messages.get_attack_message_at(index))
			{
				if (part.logMessageText.find("Constitution") != std::string::npos)
				{
					found.push_back(part.logMessageText);
				}
			}
		}
		return found;
	}

	static constexpr int BASE_HP = 25;
	static constexpr int BONUS_AT_17 = 3;
	static constexpr int BONUS_AT_18 = 4;
	static constexpr int NON_WARRIOR_CAP = 2;

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	std::unique_ptr<Player> player{};
};

// The bonus is applied on the first tick, silently: nothing changed, something
// was accounted for.
TEST_F(ConstitutionTrackerTest, FirstTickAppliesTheBonusWithoutReportingAChange)
{
	make_player("Fighter");
	ASSERT_EQ(player->get_constitution(), 17);
	ASSERT_EQ(player->get_max_hp(), BASE_HP);

	player->update_constitution_bonus(ctx);

	EXPECT_EQ(player->get_max_hp(), BASE_HP + BONUS_AT_17);
	EXPECT_TRUE(constitution_messages().empty())
		<< "reported a change from a score the character never had: " << constitution_messages().front();
}

// A second tick with nothing moved changes nothing and says nothing.
TEST_F(ConstitutionTrackerTest, ASteadyScoreIsSilent)
{
	make_player("Fighter");
	player->update_constitution_bonus(ctx);
	player->update_constitution_bonus(ctx);

	EXPECT_EQ(player->get_max_hp(), BASE_HP + BONUS_AT_17);
	EXPECT_TRUE(constitution_messages().empty());
}

// A real change is reported between the two scores the character held.
TEST_F(ConstitutionTrackerTest, ARealChangeIsReportedBetweenItsTwoScores)
{
	make_player("Fighter");
	player->update_constitution_bonus(ctx);
	player->adjust_constitution(1);

	player->update_constitution_bonus(ctx);

	EXPECT_EQ(player->get_max_hp(), BASE_HP + BONUS_AT_18);
	ASSERT_EQ(constitution_messages().size(), 1u);
	EXPECT_NE(constitution_messages().front().find("from 17 to 18"), std::string::npos)
		<< constitution_messages().front();
}

// Table 3 gives 17 a +3 and 18 a +4 for warriors only; everyone else stops at +2.
TEST_F(ConstitutionTrackerTest, ANonWarriorStopsAtPlusTwo)
{
	make_player("Wizard");

	player->update_constitution_bonus(ctx);

	EXPECT_EQ(player->get_max_hp(), BASE_HP + NON_WARRIOR_CAP)
		<< "a wizard with Con 17 was handed the warrior bonus";
}

// The cap is on the bonus, never on the penalty: a low score hurts every class alike.
TEST_F(ConstitutionTrackerTest, ThePenaltyIsNotCapped)
{
	mock.dice.clear_fixed_rolls();
	for (int die = 0; die < 6; ++die)
	{
		mock.dice.set_next_roll(3);
	}
	for (int die = 0; die < 3; ++die)
	{
		mock.dice.set_next_roll(1);
	}
	for (int die = 0; die < 9; ++die)
	{
		mock.dice.set_next_roll(3);
	}
	mock.dice.set_next_roll(5);
	make_player("Wizard");
	ASSERT_EQ(player->get_constitution(), 3);

	player->update_constitution_bonus(ctx);

	EXPECT_EQ(player->get_max_hp(), BASE_HP - 2);
}
