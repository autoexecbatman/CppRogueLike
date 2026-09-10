// Checks that saving monsters.json and reading it back preserves what was authored.
//
// What it is for. The monster editor is how content is made in this project, and it
// writes through MonsterCreator::save, which rebuilds the file from scratch rather than
// editing it. Any field the encoder does not write is therefore destroyed the first time
// anyone saves, with nothing failing and nothing logged. A parser that accepts a key the
// encoder does not emit is that bug already present.
//
// What it deliberately does not check. Not that the file's bytes are stable - key order
// and indentation are the serialiser's business. Not that values are correct, only that
// they survive. A wrong hit dice count that round-trips passes here and is caught by
// MonsterCreatorTest.
//
// The oracle is parse_full_params, which is the only schema this data has: every key the
// parser reads is a key the encoder must write. Expected values are read from
// data/content/monsters.json by hand, not from what the encoder currently produces.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterRoundTripTest.*

#include "src/Factories/MonsterCreator.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

class MonsterRoundTripTest : public ::testing::Test
{
protected:
	std::filesystem::path roundTrip;

	void SetUp() override
	{
		MonsterCreator::load("data/content/monsters.json");
		roundTrip = std::filesystem::temp_directory_path() / "monsters_roundtrip_fields.json";
	}

	void TearDown() override
	{
		// Leave the shared registry holding the real file, not the temporary one.
		MonsterCreator::load("data/content/monsters.json");
		std::filesystem::remove(roundTrip);
	}

	void save_and_reload()
	{
		MonsterCreator::save(roundTrip.string());
		MonsterCreator::load(roundTrip.string());
	}
};

// data/content/monsters.json gives dungeon_jailer the display name "Dungeon Jailer".
// The parser reads an optional "name" key, so the encoder must write one.
TEST_F(MonsterRoundTripTest, AuthoredDisplayNameSurvivesASave)
{
	ASSERT_EQ(MonsterCreator::get_params("dungeon_jailer").name, "Dungeon Jailer")
		<< "precondition: the authored name is loaded in the first place";

	save_and_reload();

	EXPECT_EQ(MonsterCreator::get_params("dungeon_jailer").name, "Dungeon Jailer");
	EXPECT_EQ(MonsterCreator::get_params("dungeon_warden").name, "Dungeon Warden");
}

// The five class-based creatures persist a tile and a colour only; MonsterCreator::load
// builds the rest in code, so their other fields are not expected to survive a file.
bool is_class_based(const std::string& key)
{
	return key == "mimic" || key == "shopkeeper" || key == "spider_small"
		|| key == "spider_giant" || key == "spider_weaver";
}

void expect_same_dice(const std::string& what, const DiceExpr& before, const DiceExpr& after)
{
	EXPECT_EQ(before.num, after.num) << what << ".num";
	EXPECT_EQ(before.sides, after.sides) << what << ".sides";
	EXPECT_EQ(before.bonus, after.bonus) << what << ".bonus";
}

// Every field MonsterParams declares, named one at a time. A dropped key fails here
// pointing at the field, which is what a count of surviving keys could not do.
void expect_same_monster(const std::string& key, const MonsterParams& before, const MonsterParams& after)
{
	EXPECT_EQ(before.name, after.name) << key << ".name";
	EXPECT_EQ(before.corpseName, after.corpseName) << key << ".corpseName";
	EXPECT_EQ(before.color, after.color) << key << ".color";
	EXPECT_EQ(before.symbol.sheet, after.symbol.sheet) << key << ".symbol.sheet";
	EXPECT_EQ(before.symbol.col, after.symbol.col) << key << ".symbol.col";
	EXPECT_EQ(before.symbol.row, after.symbol.row) << key << ".symbol.row";

	EXPECT_EQ(before.thaco, after.thaco) << key << ".thaco";
	EXPECT_EQ(before.ac, after.ac) << key << ".ac";
	EXPECT_EQ(before.xp, after.xp) << key << ".xp";
	EXPECT_EQ(before.dr, after.dr) << key << ".dr";
	EXPECT_EQ(before.morale, after.morale) << key << ".morale";
	EXPECT_EQ(before.undead, after.undead) << key << ".undead";
	EXPECT_EQ(before.ethics, after.ethics) << key << ".ethics";
	EXPECT_EQ(before.morality, after.morality) << key << ".morality";
	EXPECT_EQ(before.corpseWeight, after.corpseWeight) << key << ".corpseWeight";

	expect_same_dice(key + ".hpDice", before.hpDice, after.hpDice);
	expect_same_dice(key + ".strDice", before.strDice, after.strDice);
	expect_same_dice(key + ".dexDice", before.dexDice, after.dexDice);
	expect_same_dice(key + ".conDice", before.conDice, after.conDice);
	expect_same_dice(key + ".intDice", before.intDice, after.intDice);
	expect_same_dice(key + ".wisDice", before.wisDice, after.wisDice);
	expect_same_dice(key + ".chaDice", before.chaDice, after.chaDice);

	EXPECT_EQ(before.damage.minDamage, after.damage.minDamage) << key << ".damage.minDamage";
	EXPECT_EQ(before.damage.maxDamage, after.damage.maxDamage) << key << ".damage.maxDamage";
	EXPECT_EQ(before.damage.displayRoll, after.damage.displayRoll) << key << ".damage.displayRoll";
	EXPECT_EQ(before.damage.damageType, after.damage.damageType) << key << ".damage.damageType";

	EXPECT_EQ(before.naturalAttack, after.naturalAttack) << key << ".naturalAttack";
	EXPECT_EQ(before.bodyPlanName, after.bodyPlanName) << key << ".bodyPlanName";

	ASSERT_EQ(before.equipment.size(), after.equipment.size()) << key << ".equipment.size";
	for (size_t slot = 0; slot < before.equipment.size(); ++slot)
	{
		EXPECT_EQ(before.equipment[slot].itemKey, after.equipment[slot].itemKey)
			<< key << ".equipment[" << slot << "].itemKey";
		EXPECT_EQ(before.equipment[slot].slot, after.equipment[slot].slot)
			<< key << ".equipment[" << slot << "].slot";
	}

	EXPECT_EQ(before.aiType, after.aiType) << key << ".aiType";
	EXPECT_EQ(before.canSwim, after.canSwim) << key << ".canSwim";
	EXPECT_EQ(before.baseWeight, after.baseWeight) << key << ".baseWeight";
	EXPECT_EQ(before.levelMinimum, after.levelMinimum) << key << ".levelMinimum";
	EXPECT_EQ(before.levelMaximum, after.levelMaximum) << key << ".levelMaximum";
	EXPECT_FLOAT_EQ(before.levelScaling, after.levelScaling) << key << ".levelScaling";
}

// The whole bestiary through a save and back, field by field.
TEST_F(MonsterRoundTripTest, EveryPersistedFieldSurvivesASave)
{
	std::map<std::string, MonsterParams> before;
	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		if (!is_class_based(key))
		{
			before.emplace(key, MonsterCreator::get_params(key));
		}
	}
	ASSERT_FALSE(before.empty()) << "no monsters loaded; the test would pass vacuously";

	save_and_reload();

	for (const auto& [key, original] : before)
	{
		expect_same_monster(key, original, MonsterCreator::get_params(key));
	}
}
