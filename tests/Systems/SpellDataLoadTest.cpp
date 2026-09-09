// file: SpellDataLoadTest.cpp
// Verifies that SpellSystem::load refuses a spell whose class or effect is not a
// value the game knows, rather than substituting a default.
//
// parse_class and parse_effect_type used to end in a catch-all else, so a typo
// in spells.json became SpellClass::BOTH or SpellEffectType::NONE with nothing
// reported. The parser is the only schema this data has.
//
// The bad files are written to the system temp directory and removed afterwards,
// so nothing in the repository is touched.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "src/Systems/SpellSystem.h"

class SpellDataLoadTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		std::error_code ignored;
		std::filesystem::remove(scratchFile, ignored);

		// Leave the registry holding real data for whatever runs next.
		SpellSystem::load("data/content/spells.json");
	}

	// Writes one spell definition and returns the path it was written to.
	//
	// The key is deliberately not one of SPELL_KEYS: a builtin spell's effect is
	// compiled in rather than read from data, so only a custom key exercises
	// parse_effect_type at all.
	std::string write_spell(std::string_view spellClass, std::string_view effect)
	{
		std::ofstream out(scratchFile);
		out << R"({ "test_only_spell": { "name": "Test", "level": 1, "class": ")"
			<< spellClass << R"(", "effect": ")" << effect
			<< R"(", "description": "test" } })";
		return scratchFile.string();
	}

	std::filesystem::path scratchFile{
		std::filesystem::temp_directory_path() / "rogue_spell_load_test.json"
	};
};

// The spells the game ships with must all be readable.
TEST_F(SpellDataLoadTest, ShippedSpellDataLoads)
{
	EXPECT_NO_THROW(SpellSystem::load("data/content/spells.json"));
}

// A class the game does not know is a typo, not a request for a default.
TEST_F(SpellDataLoadTest, UnknownSpellClassIsRefused)
{
	const std::string path = write_spell("clric", "bless");

	EXPECT_THROW(SpellSystem::load(path), std::runtime_error);
}

// Same for the effect, which used to become SpellEffectType::NONE.
TEST_F(SpellDataLoadTest, UnknownSpellEffectIsRefused)
{
	const std::string path = write_spell("cleric", "firebal");

	EXPECT_THROW(SpellSystem::load(path), std::runtime_error);
}

// The three real class values still load.
TEST_F(SpellDataLoadTest, EveryKnownClassLoads)
{
	for (const auto* spellClass : { "cleric", "wizard", "both" })
	{
		const std::string path = write_spell(spellClass, "bless");

		EXPECT_NO_THROW(SpellSystem::load(path)) << "class '" << spellClass << "' was refused";
	}
}
