// Checks that a spell's class and effect survive being written to spells.json and read
// back.
//
// What it is for. Both are stored as strings through encode_class/parse_class and
// encode_effect_type/parse_effect_type, and both parsers throw on anything they do not
// recognise. If an encoder ever emits a spelling its parser rejects, saving a spell makes
// the next load throw - and that is not hypothetical: PROTECTION_FROM_EVIL was serialising
// as "none" through a duplicated encoder earlier in this project's history.
//
// Why this shape rather than a key-presence round-trip. Every field in spells.json is read
// with j.at(), so a key the encoder stopped writing throws at load on its own and needs no
// test. The codecs are the part that can be wrong while every key is present. They live in
// an anonymous namespace, so save and load are the only way to reach them.
//
// The oracle is the loaded definition itself: whatever a spell's class and effect were
// before the save, they must be the same after. No expected value is written down, because
// the claim is about preservation rather than about any particular spell.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpellCodecRoundTripTest.*

#include "src/Systems/SpellSystem.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

class SpellCodecRoundTripTest : public ::testing::Test
{
protected:
	std::filesystem::path roundTrip;

	void SetUp() override
	{
		SpellSystem::load("data/content/spells.json");
		roundTrip = std::filesystem::temp_directory_path() / "spells_roundtrip.json";
	}

	void TearDown() override
	{
		// Leave the shared registry holding the real file, not the temporary one.
		SpellSystem::load("data/content/spells.json");
		std::filesystem::remove(roundTrip);
	}
};

// Every spell keeps its class and its effect across a save. A codec pair that disagrees
// shows up either as a changed value or as a throw from the parser on the way back in.
TEST_F(SpellCodecRoundTripTest, ClassAndEffectSurviveASave)
{
	struct Encoded
	{
		SpellClass spellClass{};
		SpellEffectType effect{};
	};

	std::map<std::string, Encoded> before;
	for (const std::string& key : SpellSystem::get_all_keys())
	{
		const SpellDefinition& definition = SpellSystem::get_by_key(key);
		before.emplace(key, Encoded{ definition.spellClass, definition.effect_type });
	}
	ASSERT_FALSE(before.empty()) << "no spells loaded; the check would be vacuous";

	SpellSystem::save(roundTrip.string());
	SpellSystem::load(roundTrip.string());

	for (const auto& [key, encoded] : before)
	{
		const SpellDefinition& reloaded = SpellSystem::get_by_key(key);
		EXPECT_EQ(reloaded.spellClass, encoded.spellClass) << key << ".spellClass";
		EXPECT_EQ(reloaded.effect_type, encoded.effect) << key << ".effect_type";
	}
}

// Every spell keeps its name, level and description too - the fields the codecs do not
// touch, so that a failure above points at a codec rather than at the file.
TEST_F(SpellCodecRoundTripTest, TheRestOfTheDefinitionSurvivesToo)
{
	std::map<std::string, SpellDefinition> before;
	for (const std::string& key : SpellSystem::get_all_keys())
	{
		before.emplace(key, SpellSystem::get_by_key(key));
	}

	SpellSystem::save(roundTrip.string());
	SpellSystem::load(roundTrip.string());

	for (const auto& [key, definition] : before)
	{
		const SpellDefinition& reloaded = SpellSystem::get_by_key(key);
		EXPECT_EQ(reloaded.name, definition.name) << key << ".name";
		EXPECT_EQ(reloaded.level, definition.level) << key << ".level";
		EXPECT_EQ(reloaded.description, definition.description) << key << ".description";
	}
}
