// file: EncounterPlannerTest.cpp
// What a room is worth spending, and what gets spent on it.
//
// Both rules sat in an anonymous namespace where nothing could reach them, so
// the budget table and the two-phase selection - which are the encounter, the
// rest is placement - had never been checked. They are declared in the header
// now and asserted here.
//
// The claims come from the contract rather than from a recorded run: an entrance
// is worth nothing, a danger room is worth more than a standard one at the same
// depth, and a selection never spends past its budget nor returns past its cap.
// The room multipliers are asserted as relationships, so rebalancing the base
// experience per level leaves them alone while changing what a danger room is
// worth does not.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EncounterPlannerTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include "src/DungeonRoom.h"
#include "src/EncounterPlanner.h"
#include "src/RandomDice.h"

namespace
{
constexpr int SHALLOW = 1;
constexpr int DEEP = 4;
constexpr int CAP = 10;

// What the selection actually spent, so a budget claim is about money and not
// about how many keys came back.
int spent(const std::vector<std::string>& selected, const std::vector<MonsterCandidate>& candidates)
{
	int total = 0;
	for (const std::string& key : selected)
	{
		auto matches = [&key](const MonsterCandidate& candidate)
		{
			return candidate.key == key;
		};
		const auto found = std::ranges::find_if(candidates, matches);
		total += found->xpCost;
	}
	return total;
}
} // namespace

class EncounterPlannerTest : public ::testing::Test
{
protected:
	RandomDice dice;
};

// An entrance is where the player arrives, so nothing waits there - at any depth.
TEST_F(EncounterPlannerTest, AnEntranceIsWorthNothingAtAnyDepth)
{
	EXPECT_EQ(encounter_budget(RoomType::ENTRANCE, SHALLOW), 0);
	EXPECT_EQ(encounter_budget(RoomType::ENTRANCE, DEEP), 0);
}

// Deeper is worth more, in proportion to the depth.
TEST_F(EncounterPlannerTest, ABudgetGrowsWithTheDepth)
{
	const int shallow = encounter_budget(RoomType::STANDARD, SHALLOW);

	ASSERT_GT(shallow, 0) << "a standard room at the first level is worth nothing to fill";
	EXPECT_EQ(encounter_budget(RoomType::STANDARD, DEEP), shallow * DEEP);
}

// The room types rank the way their names promise, at the same depth.
TEST_F(EncounterPlannerTest, ADangerRoomIsWorthMostAndATreasureRoomSitsBetween)
{
	const int standard = encounter_budget(RoomType::STANDARD, DEEP);
	const int danger = encounter_budget(RoomType::DANGER, DEEP);
	const int treasure = encounter_budget(RoomType::TREASURE, DEEP);

	EXPECT_GT(danger, treasure) << "a danger room is not the most dangerous";
	EXPECT_GT(treasure, standard) << "a treasure room is not guarded more than an ordinary one";
}

// The budget is a ceiling, not a target: whatever is chosen has to fit inside it.
TEST_F(EncounterPlannerTest, ASelectionNeverSpendsPastItsBudget)
{
	const std::vector<MonsterCandidate> candidates{ { "cheap", 10 }, { "middling", 35 }, { "dear", 90 } };
	const int budget = 100;

	const std::vector<std::string> selected = select_encounter(candidates, budget, CAP, dice);

	EXPECT_LE(spent(selected, candidates), budget) << "the room cost more than it was worth";
}

// And the cap is a ceiling too, however much money is left.
TEST_F(EncounterPlannerTest, ASelectionNeverReturnsPastItsCap)
{
	const std::vector<MonsterCandidate> candidates{ { "cheap", 1 } };
	constexpr int smallCap = 3;

	const std::vector<std::string> selected = select_encounter(candidates, 10000, smallCap, dice);

	EXPECT_EQ(selected.size(), static_cast<size_t>(smallCap)) << "a huge purse ignored the cap";
}

// The cap binds the first pass too. With more affordable kinds than the room may
// hold, taking one of each would walk straight past it - which a short candidate
// list can never show.
TEST_F(EncounterPlannerTest, TheCapBindsWhileTakingOneOfEachKind)
{
	std::vector<MonsterCandidate> manyKinds;
	for (int kind = 0; kind < 12; ++kind)
	{
		manyKinds.push_back({ "kind" + std::to_string(kind), 1 });
	}
	constexpr int smallCap = 3;

	const std::vector<std::string> selected = select_encounter(manyKinds, 10000, smallCap, dice);

	EXPECT_EQ(selected.size(), static_cast<size_t>(smallCap))
		<< "one of each kind was taken without counting them against the cap";
}

// The first pass takes one of each kind it can afford, which is what makes a
// room varied rather than five of the same thing.
TEST_F(EncounterPlannerTest, EveryAffordableKindAppearsWhenTheBudgetAllowsIt)
{
	const std::vector<MonsterCandidate> candidates{ { "a", 10 }, { "b", 20 }, { "c", 30 } };

	const std::vector<std::string> selected = select_encounter(candidates, 60, CAP, dice);

	for (const MonsterCandidate& candidate : candidates)
	{
		EXPECT_NE(std::ranges::find(selected, candidate.key), selected.end())
			<< candidate.key << " was left out of a budget that covered every kind";
	}
}

// Nothing to choose from, or nothing affordable: an empty room rather than a
// free monster.
TEST_F(EncounterPlannerTest, NothingAffordableMeansAnEmptyRoom)
{
	const std::vector<MonsterCandidate> none{};
	EXPECT_TRUE(select_encounter(none, 500, CAP, dice).empty());

	const std::vector<MonsterCandidate> tooDear{ { "dear", 500 } };
	EXPECT_TRUE(select_encounter(tooDear, 499, CAP, dice).empty()) << "something was bought without the money for it";
}

// Once every kind has been taken, what is left is spent on more of them.
TEST_F(EncounterPlannerTest, LeftoverBudgetBuysMoreOfWhatIsAffordable)
{
	const std::vector<MonsterCandidate> candidates{ { "cheap", 10 } };

	const std::vector<std::string> selected = select_encounter(candidates, 45, CAP, dice);

	EXPECT_EQ(selected.size(), 4u) << "forty-five bought other than four at ten apiece";
	EXPECT_LE(spent(selected, candidates), 45);
}

// end of file: EncounterPlannerTest.cpp
