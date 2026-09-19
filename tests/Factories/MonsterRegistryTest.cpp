// file: MonsterRegistryTest.cpp
// A MonsterRegistry is a value: each instance holds its own monsters, so an edit made
// through one - the monster editor, a test - never reaches another. Game owns the one
// the game plays with and hands it out as ctx.monsterRegistry.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterRegistryTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "src/MonsterRegistry.h"
#include "src/Paths.h"

// Lowering the goblin's THAC0 in one registry leaves another's at the file's value.
TEST(MonsterRegistryTest, AnEditToOneRegistryDoesNotReachAnother)
{
	MonsterRegistry first{};
	MonsterRegistry second{};
	first.load(Paths::MONSTERS);
	second.load(Paths::MONSTERS);
	const int shippedThaco = second.get_params("goblin").thaco;

	MonsterParams sharpened = first.get_params("goblin");
	sharpened.thaco = shippedThaco - 5;
	first.set_params("goblin", sharpened);

	EXPECT_EQ(first.get_params("goblin").thaco, shippedThaco - 5);
	EXPECT_EQ(second.get_params("goblin").thaco, shippedThaco) << "an edit to one registry reached another";
}

// A monster added to one registry is listed by that registry alone.
TEST(MonsterRegistryTest, ACustomMonsterBelongsToTheRegistryItWasAddedTo)
{
	MonsterRegistry first{};
	MonsterRegistry second{};
	first.load(Paths::MONSTERS);
	second.load(Paths::MONSTERS);

	const std::string key = first.add_custom(MonsterParams{ .name = "Registry Beast" });

	EXPECT_EQ(key, "registry_beast") << "the key is the name, lower case, spaces as underscores";
	EXPECT_TRUE(std::ranges::contains(first.get_all_keys(), key));
	EXPECT_FALSE(std::ranges::contains(second.get_all_keys(), key)) << "a custom monster reached another registry";
}

// A name already taken - by a custom monster, a standard one or a class-based one - is
// numbered from 2.
TEST(MonsterRegistryTest, ATakenNameIsNumbered)
{
	MonsterRegistry monsters{};

	EXPECT_EQ(monsters.add_custom(MonsterParams{ .name = "Registry Beast" }), "registry_beast");
	EXPECT_EQ(monsters.add_custom(MonsterParams{ .name = "Registry Beast" }), "registry_beast_2");
	EXPECT_EQ(monsters.add_custom(MonsterParams{ .name = "Goblin" }), "goblin_2");
	EXPECT_EQ(monsters.add_custom(MonsterParams{ .name = "Mimic" }), "mimic_2");
}

// Only a custom monster can be removed; a key nobody holds is an error too.
TEST(MonsterRegistryTest, ABuiltinCannotBeRemoved)
{
	MonsterRegistry monsters{};

	EXPECT_THROW(monsters.remove_custom("goblin"), std::invalid_argument);
	EXPECT_THROW(monsters.remove_custom("mimic"), std::invalid_argument);
	EXPECT_THROW(monsters.remove_custom("no_such_monster"), std::out_of_range);
}

// Loading replaces the custom monsters a registry held with the file's.
TEST(MonsterRegistryTest, ALoadReplacesTheCustomMonstersItHeld)
{
	MonsterRegistry monsters{};
	const std::string key = monsters.add_custom(MonsterParams{ .name = "Registry Beast" });

	monsters.load(Paths::MONSTERS);

	EXPECT_FALSE(std::ranges::contains(monsters.get_all_keys(), key)) << "a custom monster survived a load";
}
