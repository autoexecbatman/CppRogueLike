// file: MonsterSpawnTableTest.cpp
// The monster spawn table is the registry's, derived when a monster is drawn. Nothing
// keeps a copy of it, so an edit made in the monster editor reaches the next spawn, and
// a game loaded from a save spawns from the registry main loaded - neither waits for a
// new level to rebuild anything. The factory used to build its table once, in its
// constructor, and a loaded game never passed through the level change that rebuilt it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterSpawnTableTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/Creature.h"
#include "src/MonsterFactory.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int DUNGEON_LEVEL = 1;

// Whether a distribution lists a monster by name.
bool lists(const std::vector<MonsterPercentage>& distribution, std::string_view name)
{
	auto is_named = [name](const MonsterPercentage& entry)
	{
		return entry.name == name;
	};
	return std::ranges::any_of(distribution, is_named);
}

// The first monster the registry draws at a level: positive weight, and a depth range
// that admits the level, where a levelMaximum of 0 means no upper bound. Chosen from the
// data, since a monster named by hand may not be drawn at that level at all.
std::string first_drawable(const MonsterRegistry& monsters, int dungeonLevel)
{
	for (const std::string& key : monsters.get_all_keys())
	{
		if (monsters.is_class_key(key))
		{
			continue;
		}
		const MonsterParams& params = monsters.get_params(key);
		const bool admitsLevel = params.levelMinimum <= dungeonLevel && (params.levelMaximum == 0 || dungeonLevel <= params.levelMaximum);
		if (params.baseWeight > 0 && admitsLevel)
		{
			return key;
		}
	}
	return {};
}
} // namespace

// Silencing a monster in the registry takes it out of the very next distribution, with
// nothing rebuilt in between.
TEST(MonsterSpawnTableTest, TheDistributionIsTheRegistrysAsItStandsNow)
{
	MonsterRegistry monsters{};
	monsters.load(Paths::MONSTERS);
	const std::string target = first_drawable(monsters, DUNGEON_LEVEL);
	ASSERT_FALSE(target.empty()) << "no monster is drawable at this level to silence";
	const std::string name = monsters.get_params(target).name;
	ASSERT_TRUE(lists(MonsterFactory::get_current_distribution(DUNGEON_LEVEL, monsters), name));

	MonsterParams silenced = monsters.get_params(target);
	silenced.baseWeight = 0;
	monsters.set_params(target, silenced);

	EXPECT_FALSE(lists(MonsterFactory::get_current_distribution(DUNGEON_LEVEL, monsters), name))
		<< "silencing " << target << " never reached the distribution";
}

// A monster the editor adds joins the table at once.
TEST(MonsterSpawnTableTest, ACustomMonsterJoinsTheTable)
{
	MonsterRegistry monsters{};
	monsters.load(Paths::MONSTERS);

	[[maybe_unused]] const std::string key = monsters.add_custom(MonsterParams{ .name = "Registry Beast", .baseWeight = 10, .levelMinimum = 1 });

	EXPECT_TRUE(lists(MonsterFactory::get_current_distribution(DUNGEON_LEVEL, monsters), "Registry Beast"));
}

// A spawn draws from the registry in the ctx it is given. Two contexts, each with every
// monster but one silenced, and a roll of 1 in each: the first spawns its wolf and the
// second its bat - neither carries anything, so neither needs item data.
TEST(MonsterSpawnTableTest, ASpawnDrawsFromTheRegistryInCtx)
{
	auto spawn_with_only = [](std::string_view survivor)
	{
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		std::vector<std::unique_ptr<Creature>> creatures{};
		ctx.creatures = &creatures;
		for (const std::string& key : mock.monsterRegistry.get_all_keys())
		{
			if (mock.monsterRegistry.is_class_key(key) || key == survivor)
			{
				continue;
			}
			MonsterParams silenced = mock.monsterRegistry.get_params(key);
			silenced.baseWeight = 0;
			mock.monsterRegistry.set_params(key, silenced);
		}
		mock.dice.set_next_roll(1);

		MonsterFactory::spawn_random_monster(Vector2D{ 1, 1 }, DUNGEON_LEVEL, ctx);

		return creatures.size() == 1 ? creatures.front()->actorData.name : std::string{ "spawned nothing, or more than one" };
	};

	EXPECT_EQ(spawn_with_only("wolf"), "wolf");
	EXPECT_EQ(spawn_with_only("bat"), "bat");
}
