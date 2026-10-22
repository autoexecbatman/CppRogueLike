// file: TrapSearchTest.cpp
// Searching for traps, the `f` key: the only way a hidden trap is found.
//
// Two characters can search, each for its own kind of trap. A thief's Find/Remove Traps
// finds the small ones, dart and arrow, on d100 under the skill (PHB page 87: "Only
// small traps can be found in this way", taking 1d10 rounds). A dwarf finds a pit, a
// stonework trap, on 1-3 on 1d6 (PHB page 44). Each trap is searched once per
// experience level. The reach, the eight adjacent tiles, and the dwarf's 1 round are
// this game's stated deviations; Trap.h gives the reasons.
//
// Every roll is scripted in the order the search asks for it: each trap's roll, then
// the time. A test that expects a trap to be passed over queues a certain find ahead
// of the time die, so a search that rolled for it anyway would find it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapSearchTest.*

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "src/Actor.h"
#include "src/GameContext.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/ThiefSkills.h"
#include "src/Trap.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr Vector2D SEARCHER_TILE{ 5, 5 };
// Diagonal, so a reach that only looked straight ahead would miss it.
constexpr Vector2D ADJACENT_TILE{ 6, 6 };
constexpr Vector2D OTHER_ADJACENT_TILE{ 4, 5 };
constexpr Vector2D TWO_TILES_AWAY{ 7, 5 };
// Base 5 plus 40 bought.
constexpr int THIEF_SKILL = 45;
} // namespace

class TrapSearchTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		// Dexterity 13 is the middle of Table 28's flat row, so a rogue's percentage is
		// the base score, the race and the points, with nothing else folded in.
		searcher.set_dexterity(13);
		searcher.set_creature_level(1);
		become(CreatureClass::FIGHTER, PlayerRaceState::HUMAN);
	}

	void become(CreatureClass creatureClass, PlayerRaceState race)
	{
		searcher.set_creature_class(creatureClass);
		searcher.lifespan->race = race;
	}

	// A human thief whose Find/Remove Traps is THIEF_SKILL.
	void become_thief()
	{
		become(CreatureClass::ROGUE, PlayerRaceState::HUMAN);
		searcher.thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = 40;
		ASSERT_EQ(searcher.thief_skill(ThiefSkill::FIND_REMOVE_TRAPS), THIEF_SKILL);
	}

	void become_dwarf_fighter()
	{
		become(CreatureClass::FIGHTER, PlayerRaceState::DWARF);
	}

	Trap& place(TrapType kind, Vector2D position)
	{
		mock.traps.push_back(std::make_unique<Trap>(position, kind, mock.tile_config));
		return *mock.traps.back();
	}

	std::string all_message_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	static bool is_found(const Trap& trap)
	{
		return trap.get_state() == TrapState::DETECTED && !trap.has_state(ActorState::IS_INVISIBLE);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Player searcher{ SEARCHER_TILE };
};

// A roll at the thief's own percentage finds the dart trap and shows it, and the look
// takes the book's 1d10 rounds.
TEST_F(TrapSearchTest, AThiefFindsAnAdjacentDartTrapAtItsOwnPercentage)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	const Trap& dart = place(TrapType::DART, ADJACENT_TILE);
	mock.dice.set_next_roll(THIEF_SKILL);
	mock.dice.set_next_roll(7);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 7 });
	EXPECT_TRUE(is_found(dart));
	EXPECT_NE(all_message_text().find("You find a hidden dart trap."), std::string::npos) << all_message_text();
	EXPECT_EQ(all_message_text().find("You find no traps."), std::string::npos) << all_message_text();
}

// One over the percentage misses, the trap stays hidden, and the time is spent anyway.
TEST_F(TrapSearchTest, OneOverTheThiefsPercentageMisses)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	const Trap& arrow = place(TrapType::ARROW, ADJACENT_TILE);
	mock.dice.set_next_roll(THIEF_SKILL + 1);
	mock.dice.set_next_roll(3);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 3 });
	EXPECT_EQ(arrow.get_state(), TrapState::HIDDEN);
	EXPECT_TRUE(arrow.has_state(ActorState::IS_INVISIBLE));
	EXPECT_NE(all_message_text().find("You find no traps."), std::string::npos) << all_message_text();
}

// "Only small traps can be found in this way": a pit is not the thief's to find.
TEST_F(TrapSearchTest, AThiefCannotFindAPit)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	const Trap& pit = place(TrapType::PIT, ADJACENT_TILE);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(4);

	ASSERT_TRUE(TrapSearch::search_around(searcher, ctx).has_value());
	EXPECT_EQ(pit.get_state(), TrapState::HIDDEN) << "a thief's roll found a pit";
}

// A dwarf finds a pit on 1-3 on 1d6, and a dwarf's look takes one round.
TEST_F(TrapSearchTest, ThreeOnTheDieFindsAPitForADwarf)
{
	become_dwarf_fighter();
	const Trap& pit = place(TrapType::PIT, ADJACENT_TILE);
	mock.dice.set_next_roll(3);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 1 });
	EXPECT_TRUE(is_found(pit));
	EXPECT_NE(all_message_text().find("You find a hidden pit trap."), std::string::npos) << all_message_text();
}

TEST_F(TrapSearchTest, FourOnTheDieMissesAPit)
{
	become_dwarf_fighter();
	const Trap& pit = place(TrapType::PIT, ADJACENT_TILE);
	mock.dice.set_next_roll(4);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 1 });
	EXPECT_EQ(pit.get_state(), TrapState::HIDDEN);
}

// The dwarf's sense is for stonework; a dart trap is not stonework.
TEST_F(TrapSearchTest, ADwarfCannotFindADartTrap)
{
	become_dwarf_fighter();
	const Trap& dart = place(TrapType::DART, ADJACENT_TILE);
	mock.dice.set_next_roll(1);

	ASSERT_TRUE(TrapSearch::search_around(searcher, ctx).has_value());
	EXPECT_EQ(dart.get_state(), TrapState::HIDDEN) << "a dwarf's roll found a dart trap";
}

// A dwarf thief looks for both kinds, each with its own roll in trap order, and takes
// the thief's time.
TEST_F(TrapSearchTest, ADwarfThiefLooksForBothKindsAndTakesTheThiefsTime)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	searcher.lifespan->race = PlayerRaceState::DWARF;
	const Trap& pit = place(TrapType::PIT, ADJACENT_TILE);
	const Trap& dart = place(TrapType::DART, OTHER_ADJACENT_TILE);
	mock.dice.set_next_roll(2);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(9);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 9 });
	EXPECT_TRUE(is_found(pit));
	EXPECT_TRUE(is_found(dart));
}

// One search per trap per experience level: a second look at the same level makes no
// roll for it, and the level gained buys the next.
TEST_F(TrapSearchTest, ATrapIsSearchedOncePerLevel)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	const Trap& dart = place(TrapType::DART, ADJACENT_TILE);
	mock.dice.set_next_roll(90);
	mock.dice.set_next_roll(5);
	ASSERT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 5 });

	// A certain find is queued and must not be reached; the time die takes it.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 1 });
	EXPECT_EQ(dart.get_state(), TrapState::HIDDEN) << "the same level searched the same trap twice";

	searcher.set_creature_level(2);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(6);
	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::optional<int>{ 6 });
	EXPECT_TRUE(is_found(dart)) << "a level gained is what buys the next search";
}

// The search reaches the eight tiles around the searcher and no further.
TEST_F(TrapSearchTest, ATrapTwoTilesAwayIsNotSearched)
{
	ASSERT_NO_FATAL_FAILURE(become_thief());
	const Trap& far = place(TrapType::DART, TWO_TILES_AWAY);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(4);

	ASSERT_TRUE(TrapSearch::search_around(searcher, ctx).has_value());
	EXPECT_EQ(far.get_state(), TrapState::HIDDEN) << "a trap two tiles away was searched";
}

// Anyone who can find no kind of trap is told so and spends no turn.
TEST_F(TrapSearchTest, AHumanFighterCannotSearchAndSpendsNoTurn)
{
	const Trap& dart = place(TrapType::DART, ADJACENT_TILE);
	mock.dice.set_next_roll(1);

	EXPECT_EQ(TrapSearch::search_around(searcher, ctx), std::nullopt);
	EXPECT_EQ(dart.get_state(), TrapState::HIDDEN);
	EXPECT_NE(all_message_text().find("You don't know how to look for traps."), std::string::npos) << all_message_text();
}

// end of file: TrapSearchTest.cpp
