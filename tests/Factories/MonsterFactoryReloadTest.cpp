// Checks that the monster spawn table can be rebuilt from the registry, and that
// rebuilding it does not list every monster twice.
//
// What it is for. MonsterFactory built its table in its constructor from
// MonsterCreator's registry and nothing rebuilt it, so a monster edited in the monster
// editor could not reach the spawn table until the next launch - the same broken loop
// ItemFactory had. Map::regenerate now rebuilds both on the way into a level.
//
// The hazard that comes with it. addMonsterType is a bare push_back, so rebuilding
// without clearing lists every monster twice and doubles its weight against anything
// added later. Nothing fails: the dungeon still populates, from a table that no longer
// means what it says.
//
// The oracle is get_current_distribution, which reports one entry per monster with a
// positive weight at the given level, so a doubled table shows up as a doubled count.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterFactoryReloadTest.*

#include "src/Factories/MonsterCreator.h"
#include "src/Factories/MonsterFactory.h"
#include <gtest/gtest.h>

#include <string>

class MonsterFactoryReloadTest : public ::testing::Test
{
protected:
	static constexpr int DUNGEON_LEVEL = 1;

	void SetUp() override
	{
		MonsterCreator::load("data/content/monsters.json");
	}

	void TearDown() override
	{
		// Leave the shared registry as it was found; a weight is changed below.
		MonsterCreator::load("data/content/monsters.json");
	}
};

TEST_F(MonsterFactoryReloadTest, ReloadingDoesNotDuplicateTheTable)
{
	MonsterFactory factory;
	const size_t afterConstruction = factory.get_current_distribution(DUNGEON_LEVEL).size();
	ASSERT_GT(afterConstruction, 0u) << "no monsters in the table; the check would be vacuous";

	factory.reload_from_registry();
	factory.reload_from_registry();

	EXPECT_EQ(factory.get_current_distribution(DUNGEON_LEVEL).size(), afterConstruction)
		<< "the table grew when it was rebuilt, so every monster is listed more than once";
}

// A registry change reaches the table, which is the whole reason the reload exists.
//
// The monster to silence is chosen from the data rather than named: a monster is only in
// the level's distribution if its depth range admits that level, so picking one by hand
// risks removing something that was never counted. The orc, for instance, starts at
// depth 2 and is absent from level 1 entirely.
TEST_F(MonsterFactoryReloadTest, ReloadPicksUpAWeightChange)
{
	MonsterFactory factory;
	const size_t before = factory.get_current_distribution(DUNGEON_LEVEL).size();
	ASSERT_GT(before, 0u) << "nothing spawns at this level; the check would be vacuous";

	// Find a monster this level actually draws: positive weight, and a depth range
	// that admits DUNGEON_LEVEL. A levelMaximum of 0 means no upper bound.
	std::string target;
	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		const MonsterParams& params = MonsterCreator::get_params(key);
		const bool admitsLevel = params.levelMinimum <= DUNGEON_LEVEL
			&& (params.levelMaximum == 0 || DUNGEON_LEVEL <= params.levelMaximum);
		if (params.baseWeight > 0 && admitsLevel)
		{
			target = key;
			break;
		}
	}
	ASSERT_FALSE(target.empty()) << "no monster is drawable at this level to silence";

	// Take it out of the spawn table the way the editor would.
	MonsterParams withoutWeight = MonsterCreator::get_params(target);
	withoutWeight.baseWeight = 0;
	MonsterCreator::set_params(target, withoutWeight);

	factory.reload_from_registry();

	EXPECT_LT(factory.get_current_distribution(DUNGEON_LEVEL).size(), before)
		<< "silencing " << target << " never reached the spawn table";
}
