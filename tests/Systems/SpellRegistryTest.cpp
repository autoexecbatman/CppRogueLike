// file: SpellRegistryTest.cpp
// A SpellRegistry is a value: each instance holds its own spells, so an edit made
// through one - the spell editor, a test - never reaches another. Game owns the one
// the game plays with and hands it out as ctx.spellRegistry.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpellRegistryTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/Paths.h"
#include "src/SpellRegistry.h"

// Raising bless in one registry leaves another's bless at the level the file gave it.
TEST(SpellRegistryTest, AnEditToOneRegistryDoesNotReachAnother)
{
	SpellRegistry first{};
	SpellRegistry second{};
	first.load(Paths::SPELLS);
	second.load(Paths::SPELLS);
	const int shippedLevel = second.get_by_key("bless").level;

	SpellDefinition raised = first.get_by_key("bless");
	raised.level = shippedLevel + 3;
	first.set_by_key("bless", raised);

	EXPECT_EQ(first.get_by_key("bless").level, shippedLevel + 3);
	EXPECT_EQ(second.get_by_key("bless").level, shippedLevel) << "an edit to one registry reached another";
}

// A spell added to one registry is listed by that registry alone.
TEST(SpellRegistryTest, ACustomSpellBelongsToTheRegistryItWasAddedTo)
{
	SpellRegistry first{};
	SpellRegistry second{};
	first.load(Paths::SPELLS);
	second.load(Paths::SPELLS);

	const std::string key = first.add_custom(SpellDefinition{
		.name = "Registry Spark",
		.level = 1,
		.spellClass = SpellClass::WIZARD,
		.description = "a spell only one registry holds",
		.effect_type = SpellEffectType::MAGIC_MISSILE });

	EXPECT_EQ(key, "registry_spark") << "the key is the name, lower case, spaces as underscores";
	EXPECT_TRUE(std::ranges::contains(first.get_all_keys(), key));
	EXPECT_FALSE(std::ranges::contains(second.get_all_keys(), key)) << "a custom spell reached another registry";
}

// Casting reads only a builtin's effect, which is compiled in, so a registry that was
// never loaded still dispatches every builtin.
TEST(SpellRegistryTest, AnUnloadedRegistryKnowsEveryBuiltinsEffect)
{
	const SpellRegistry fresh{};

	EXPECT_EQ(fresh.get_by_key("fireball").effect_type, SpellEffectType::FIREBALL);
	EXPECT_EQ(fresh.get_by_key("sanctuary").effect_type, SpellEffectType::SANCTUARY);
}

// A caster may learn a spell of its own class or of both, at or below its spell level;
// custom spells come after every builtin. The expected sets are that rule applied to the
// shipped file, so an edit to the data moves them with it.
TEST(SpellRegistryTest, ACasterSeesItsClassAndBothAtOrBelowItsLevel)
{
	SpellRegistry spells{};
	spells.load(Paths::SPELLS);
	const std::string bothWays = spells.add_custom(SpellDefinition{
		.name = "Both Ways",
		.level = 1,
		.spellClass = SpellClass::BOTH,
		.description = "a spell of both classes",
		.effect_type = SpellEffectType::NONE });

	std::ifstream file(Paths::resolve(Paths::SPELLS));
	const nlohmann::json shipped = nlohmann::json::parse(file);
	auto expected_for = [&shipped, &bothWays](std::string_view spellClass, int maxSpellLevel)
	{
		std::set<std::string> keys{ bothWays };
		for (const auto& [key, record] : shipped.items())
		{
			const std::string recordClass = record.at("class").get<std::string>();
			if ((recordClass == spellClass || recordClass == "both") && record.at("level").get<int>() <= maxSpellLevel)
			{
				keys.insert(key);
			}
		}
		return keys;
	};

	const std::vector<std::string> wizardFirst = spells.get_available_spells(CasterClass::WIZARD, 1);
	const std::vector<std::string> clericSecond = spells.get_available_spells(CasterClass::CLERIC, 2);

	EXPECT_EQ(std::set<std::string>(wizardFirst.begin(), wizardFirst.end()), expected_for("wizard", 1));
	EXPECT_EQ(std::set<std::string>(clericSecond.begin(), clericSecond.end()), expected_for("cleric", 2));
	ASSERT_FALSE(wizardFirst.empty());
	EXPECT_EQ(wizardFirst.back(), bothWays) << "a custom spell is listed after every builtin";
}

// A name already taken - by a custom spell or by a builtin - is numbered from 2.
TEST(SpellRegistryTest, ATakenNameIsNumbered)
{
	SpellRegistry spells{};

	EXPECT_EQ(spells.add_custom(SpellDefinition{ .name = "Registry Spark" }), "registry_spark");
	EXPECT_EQ(spells.add_custom(SpellDefinition{ .name = "Registry Spark" }), "registry_spark_2");
	EXPECT_EQ(spells.add_custom(SpellDefinition{ .name = "Bless" }), "bless_2");
}

// Only a custom spell can be removed; a key nobody holds is an error too.
TEST(SpellRegistryTest, ABuiltinCannotBeRemoved)
{
	SpellRegistry spells{};

	EXPECT_THROW(spells.remove_custom("bless"), std::invalid_argument);
	EXPECT_THROW(spells.remove_custom("no_such_spell"), std::out_of_range);
}

// Loading replaces the custom spells a registry held with the file's.
TEST(SpellRegistryTest, ALoadReplacesTheCustomSpellsItHeld)
{
	SpellRegistry spells{};
	const std::string key = spells.add_custom(SpellDefinition{ .name = "Registry Spark" });

	spells.load(Paths::SPELLS);

	EXPECT_FALSE(std::ranges::contains(spells.get_all_keys(), key)) << "a custom spell survived a load";
}
