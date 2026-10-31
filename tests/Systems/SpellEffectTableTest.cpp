// file: SpellEffectTableTest.cpp
// Every spell effect, checked through the places that name it: the spell
// editor shows each one by the name its builtin spell has in spells.json, a
// custom spell loads and saves each one under its key, the builtin spells are
// every effect but NONE, and the editor's effect field visits each once.
//
// EFFECT_KEYS is spelled out here from the enumerators rather than read from
// the code, so a key the code gets wrong cannot also be the expectation.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpellEffectTableTest.*

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "src/Paths.h"
#include "src/SpellEditor.h"
#include "src/SpellRegistry.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
	// Every effect's key, in the order SpellEffectType declares them.
	constexpr auto EFFECT_KEYS = std::to_array<std::string_view>({
		"cure_light_wounds",
		"bless",
		"sanctuary",
		"protection_from_evil",
		"hold_person",
		"silence",
		"magic_missile",
		"shield",
		"sleep",
		"invisibility",
		"web",
		"fireball",
		"teleport",
		"knock",
		"none",
	});

	// How many effects have a builtin spell: all of them but NONE, the last.
	constexpr std::size_t BUILTIN_COUNT = EFFECT_KEYS.size() - 1;

	// The spell data the game ships.
	nlohmann::json shipped_spells()
	{
		std::ifstream in(Paths::resolve(Paths::SPELLS));
		return nlohmann::json::parse(in);
	}

	// The name the shipped data gives the builtin spell of effect row n, or
	// "None" for the NONE row, which has no builtin spell.
	std::string expected_name(std::size_t row)
	{
		if (row == BUILTIN_COUNT)
		{
			return "None";
		}
		return shipped_spells()
			.at(std::string{ EFFECT_KEYS[row] })
			.at("name")
			.get<std::string>();
	}
} // namespace

class SpellEffectTableTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		std::error_code ignored;
		std::filesystem::remove(scratchFile, ignored);
	}

	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	SpellEditorModel model{};
	std::filesystem::path scratchFile{ std::filesystem::temp_directory_path() /
		"rogue_spell_effect_table_test.json" };
};

TEST_F(SpellEffectTableTest, TheEditorNamesEveryBuiltinsEffectAsTheSpellIsNamed)
{
	for (std::size_t row = 0; row < BUILTIN_COUNT; ++row)
	{
		model.load(ctx, EFFECT_KEYS[row]);

		EXPECT_EQ(
			model.field_value(SpellEditorModel::FieldId::EFFECT_TYPE),
			expected_name(row))
			<< EFFECT_KEYS[row];
	}
}

TEST_F(SpellEffectTableTest, TheEditorNamesNoEffectNone)
{
	const std::string key = model.add_record(ctx);

	model.load(ctx, key);

	EXPECT_EQ(
		model.field_value(SpellEditorModel::FieldId::EFFECT_TYPE),
		"None");
}

TEST_F(SpellEffectTableTest, EveryEffectLoadsAndSavesUnderItsKey)
{
	for (std::size_t row = 0; row < EFFECT_KEYS.size(); ++row)
	{
		nlohmann::json root = shipped_spells();
		root["test_only_spell"] = {
			{ "name", "Test" },
			{ "level", 1 },
			{ "class", "wizard" },
			{ "effect", EFFECT_KEYS[row] },
			{ "description", "test" },
		};
		{
			std::ofstream out(scratchFile);
			out << root.dump();
		}
		SpellRegistry spells{};

		spells.load(scratchFile.string());
		const SpellEffectType loaded =
			spells.get_by_key("test_only_spell").effect_type;
		spells.save(scratchFile.string());
		std::ifstream in(scratchFile);
		const nlohmann::json saved = nlohmann::json::parse(in);

		EXPECT_EQ(loaded, static_cast<SpellEffectType>(row))
			<< EFFECT_KEYS[row];
		EXPECT_EQ(
			saved.at("test_only_spell").at("effect").get<std::string>(),
			EFFECT_KEYS[row]);
	}
}

TEST_F(SpellEffectTableTest, TheBuiltinsAreEveryEffectButNoneInEnumOrder)
{
	const SpellRegistry fresh{};
	const std::vector<std::string> keys = fresh.get_all_keys();
	ASSERT_EQ(keys.size(), BUILTIN_COUNT)
		<< "an unloaded registry holds only the builtins";

	for (std::size_t row = 0; row < BUILTIN_COUNT; ++row)
	{
		EXPECT_EQ(keys[row], EFFECT_KEYS[row]);
		EXPECT_EQ(
			fresh.get_by_key(keys[row]).effect_type,
			static_cast<SpellEffectType>(row))
			<< keys[row];
	}
}

TEST_F(SpellEffectTableTest, TheEffectFieldCyclesThroughEveryEffectOnce)
{
	using Field = SpellEditorModel::FieldId;
	const std::string key = model.add_record(ctx);
	model.load(ctx, key);
	std::vector<std::string> shown{};

	for (std::size_t step = 0; step < EFFECT_KEYS.size(); ++step)
	{
		model.field_adjust(Field::EFFECT_TYPE, 1);
		shown.push_back(model.field_value(Field::EFFECT_TYPE));
	}

	// From NONE, a step forward reaches the first effect, and the last step
	// wraps back to NONE.
	ASSERT_EQ(shown.size(), EFFECT_KEYS.size());
	for (std::size_t row = 0; row < EFFECT_KEYS.size(); ++row)
	{
		EXPECT_EQ(shown[row], expected_name(row)) << "step " << row + 1;
	}
	model.field_adjust(Field::EFFECT_TYPE, 1);
	model.field_adjust(Field::EFFECT_TYPE, -1);
	EXPECT_EQ(model.field_value(Field::EFFECT_TYPE), "None")
		<< "back from the first effect wraps to NONE";
}
