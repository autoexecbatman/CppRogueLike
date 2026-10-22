// Checks that saving items.json and reading it back preserves what was authored.
//
// What it is for. items.json is the file this project has already lost once: the data
// used snake_case keys while the parser read camelCase, every field silently took its
// default, and create_random_of_category returned nullptr forever because baseWeight came
// back 0. Nothing threw and nothing logged. Every key is now required, and this checks the
// other half: that the encoder writes every key the parser requires.
//
// What it deliberately does not check. Not that values are correct, only that they
// survive. Not the enhanced spawn rules, which load from a separate file through
// load_enhanced_rules and have their own schema.
//
// The oracle is the registry's parser, the only schema this data has: every key the parser
// reads is a key the encoder must write. Expected values come from the entry loaded
// before the save, never from what the encoder produces.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemRoundTripTest.*

#include "src/DiceExpr.h"
#include "src/ItemRegistry.h"
#include "src/Paths.h"
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

class ItemRoundTripTest : public ::testing::Test
{
protected:
	std::filesystem::path roundTrip;
	ItemRegistry items{};

	void SetUp() override
	{
		items.load("data/content/items.json");
		items.load_enhanced_rules("data/content/enhanced_rules.json");
		roundTrip = std::filesystem::temp_directory_path() / "items_roundtrip.json";
	}

	void TearDown() override
	{
		std::filesystem::remove(roundTrip);
	}
};

// Every field ItemParams declares, named one at a time, so a lost key fails pointing at
// the field rather than at a count.
void expect_same_item(const std::string& key, const ItemParams& before, const ItemParams& after)
{
	EXPECT_EQ(before.name, after.name) << key << ".name";
	EXPECT_EQ(before.category, after.category) << key << ".category";
	EXPECT_EQ(before.color, after.color) << key << ".color";
	EXPECT_EQ(before.itemClass, after.itemClass) << key << ".itemClass";
	EXPECT_EQ(before.value, after.value) << key << ".value";
	EXPECT_EQ(before.pickableType, after.pickableType) << key << ".pickableType";

	EXPECT_EQ(to_text(before.consumableAmount), to_text(after.consumableAmount)) << key << ".consumableAmount";
	EXPECT_EQ(before.range, after.range) << key << ".range";
	EXPECT_EQ(before.damage, after.damage) << key << ".damage";
	EXPECT_EQ(before.confuseTurns, after.confuseTurns) << key << ".confuseTurns";
	EXPECT_EQ(to_text(before.duration), to_text(after.duration)) << key << ".duration";
	EXPECT_EQ(before.durationUnit, after.durationUnit) << key << ".durationUnit";

	EXPECT_EQ(before.effect, after.effect) << key << ".effect";
	EXPECT_EQ(before.effectBonus, after.effectBonus) << key << ".effectBonus";

	EXPECT_EQ(before.strBonus, after.strBonus) << key << ".strBonus";
	EXPECT_EQ(before.dexBonus, after.dexBonus) << key << ".dexBonus";
	EXPECT_EQ(before.conBonus, after.conBonus) << key << ".conBonus";
	EXPECT_EQ(before.intBonus, after.intBonus) << key << ".intBonus";
	EXPECT_EQ(before.wisBonus, after.wisBonus) << key << ".wisBonus";
	EXPECT_EQ(before.chaBonus, after.chaBonus) << key << ".chaBonus";
	EXPECT_EQ(before.application, after.application) << key << ".application";
	EXPECT_EQ(before.exceptionalStrength, after.exceptionalStrength) << key << ".exceptionalStrength";

	EXPECT_EQ(before.nutritionValue, after.nutritionValue) << key << ".nutritionValue";
	EXPECT_EQ(before.acBonus, after.acBonus) << key << ".acBonus";

	EXPECT_EQ(before.ranged, after.ranged) << key << ".ranged";
	EXPECT_EQ(before.handRequirement, after.handRequirement) << key << ".handRequirement";
	EXPECT_EQ(before.weaponSize, after.weaponSize) << key << ".weaponSize";
	EXPECT_EQ(before.strengthRating, after.strengthRating) << key << ".strengthRating";

	EXPECT_EQ(before.consumableEffect, after.consumableEffect) << key << ".consumableEffect";
	EXPECT_EQ(before.consumableBuffType, after.consumableBuffType) << key << ".consumableBuffType";
	EXPECT_EQ(before.targetMode, after.targetMode) << key << ".targetMode";
	EXPECT_EQ(before.scrollAnimation, after.scrollAnimation) << key << ".scrollAnimation";

	EXPECT_EQ(before.baseWeight, after.baseWeight) << key << ".baseWeight";
	EXPECT_EQ(before.weight, after.weight) << key << ".weight";
	EXPECT_EQ(before.levelMin, after.levelMin) << key << ".levelMin";
	EXPECT_EQ(before.levelMax, after.levelMax) << key << ".levelMax";
	EXPECT_FLOAT_EQ(before.levelScaling, after.levelScaling) << key << ".levelScaling";
}

TEST_F(ItemRoundTripTest, EveryPersistedFieldSurvivesASave)
{
	std::map<std::string, ItemParams> before;
	for (const std::string& key : items.get_all_keys())
	{
		before.emplace(key, items.get_params(key));
	}
	ASSERT_FALSE(before.empty()) << "no items loaded; the test would pass vacuously";

	items.save(roundTrip.string());
	items.load(roundTrip.string());

	for (const auto& [key, params] : before)
	{
		expect_same_item(key, params, items.get_params(key));
	}
}

// Writes the real file to the temporary path with one key removed from one record, and
// returns what loading it threw. Fails the calling test if nothing was thrown.
std::string load_without_field(
	ItemRegistry& items,
	const std::filesystem::path& destination,
	const std::string& itemKey,
	const std::string& field)
{
	std::ifstream source(Paths::resolve(Paths::ITEMS));
	EXPECT_TRUE(source.is_open()) << "cannot read the real items file";
	nlohmann::json root = nlohmann::json::parse(source);
	EXPECT_TRUE(root.contains(itemKey)) << itemKey << " must exist to be damaged";
	EXPECT_TRUE(root[itemKey].contains(field)) << itemKey << " has no " << field << " to remove";
	root[itemKey].erase(field);

	std::ofstream damaged(destination);
	damaged << root.dump(2);
	damaged.close();

	try
	{
		items.load(destination.string());
	}
	catch (const std::runtime_error& refusal)
	{
		return refusal.what();
	}
	ADD_FAILURE() << "loading a record with no " << field << " threw nothing";
	return {};
}

// The encoder writes every key unconditionally and every record carries every key, so
// a record missing one is a corrupted or hand-edited file. Loading it
// must fail loudly: the alternative is the registry filling with items whose every value
// is a default, which is the failure this file exists for.
//
// Every field is tried rather than a chosen one, because the defect this replaces was a
// property of one line at a time - every other read correct and one silent default is
// exactly the state that produced the original bug.
TEST_F(ItemRoundTripTest, EveryFieldOfARecordIsRequired)
{
	std::ifstream source(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(source.is_open()) << "cannot read the real items file";
	const nlohmann::json root = nlohmann::json::parse(source);
	ASSERT_TRUE(root.contains("health_potion")) << "health_potion must exist to be damaged";

	std::set<std::string> fields;
	for (const auto& [field, unused] : root.at("health_potion").items())
	{
		fields.insert(field);
	}
	ASSERT_FALSE(fields.empty()) << "no fields to remove; the test would pass vacuously";

	for (const std::string& field : fields)
	{
		const std::string refusal = load_without_field(items, roundTrip, "health_potion", field);

		const std::size_t namesRecord = refusal.find("health_potion");
		const std::size_t namesField = refusal.find(field);
		EXPECT_NE(namesRecord, std::string::npos)
			<< "the refusal must name the record, since 98 of them load: " << refusal;
		EXPECT_NE(namesField, std::string::npos)
			<< "the refusal must name " << field << ": " << refusal;
		// Both names present in either order reads as a sentence about the wrong thing,
		// so the record has to come first.
		EXPECT_LT(namesRecord, namesField)
			<< "the refusal names " << field << " before the record it belongs to: " << refusal;
	}
}

// Paired with the refusals above: they must be about the missing key rather than about
// the file, or they would pass while rejecting everything.
TEST_F(ItemRoundTripTest, TheRealFileLoadsWithoutThrowing)
{
	EXPECT_NO_THROW(items.load("data/content/items.json"));
	EXPECT_FALSE(items.get_all_keys().empty());
}

// A category is only consulted through create_random_of_category, which also requires a
// positive baseWeight - so an item with a category and no weight has configuration that
// can never fire, and the category is a lie about how the item is obtained.
//
// This is narrower than "every item is reachable", which cannot be checked here: an item
// may also be produced by an enhanced rule pool or by a hardcoded ItemCreator::create
// call, and the second is not visible from a test.
TEST_F(ItemRoundTripTest, ACategoryImpliesTheItemCanBeDrawnFromIt)
{
	for (const std::string& key : items.get_all_keys())
	{
		const ItemParams& params = items.get_params(key);
		if (params.category.empty())
		{
			continue;
		}

		EXPECT_GT(params.baseWeight, 0)
			<< key << " is in category '" << params.category
			<< "' but has no spawn weight, so that category can never draw it";
	}
}

// Exactly these nine items set a score rather than add to it. The list is the data as it
// stood when this key was introduced, so converting it cannot change which items set. The book
// agrees for the girdles, the gauntlets and the potion: gauntlets of ogre power imbue
// "18/00 Strength" (DMG, PDF page 964), a girdle of giant strength's Strength "is not
// cumulative" (PDF page 966), and the potion grants a giant's strength (PDF page 890).
// The amulet of ogre power has no DMG entry; it is the game's own item.
TEST_F(ItemRoundTripTest, ExactlyTheGiantAndOgreStrengthItemsSetTheirScore)
{
	const std::set<std::string> setting{
		"amulet_of_ogre_power",
		"gauntlets_of_ogre_power",
		"girdle_of_cloud_giant_strength",
		"girdle_of_fire_giant_strength",
		"girdle_of_frost_giant_strength",
		"girdle_of_hill_giant_strength",
		"girdle_of_stone_giant_strength",
		"girdle_of_storm_giant_strength",
		"potion_of_giant_strength",
	};
	std::ifstream file(Paths::resolve(Paths::ITEMS));
	const nlohmann::json root = nlohmann::json::parse(file);

	for (const auto& [key, record] : root.items())
	{
		ASSERT_TRUE(record.contains("application")) << key << " names no application";
		EXPECT_EQ(record.at("application"), setting.contains(key) ? "set" : "add") << key;
	}
}

// A record naming an application this build does not know is refused rather than read as
// adding, which is what a silent default would make of it.
TEST_F(ItemRoundTripTest, AnUnknownApplicationIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(source.is_open()) << "cannot read the real items file";
	nlohmann::json root = nlohmann::json::parse(source);
	root.at("health_potion")["application"] = "sideways";
	std::ofstream(roundTrip) << root.dump(4);

	ItemRegistry damaged{};
	EXPECT_ANY_THROW(damaged.load(roundTrip.string()));
}

// A potion's duration that is not a dice expression is refused rather than read as none.
TEST_F(ItemRoundTripTest, ADurationThatIsNotDiceIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(source.is_open()) << "cannot read the real items file";
	nlohmann::json root = nlohmann::json::parse(source);
	root.at("invisibility_potion")["duration"] = "d4";
	std::ofstream(roundTrip) << root.dump(4);

	ItemRegistry damaged{};
	EXPECT_ANY_THROW(damaged.load(roundTrip.string()));
}

// A duration with no unit, or a unit this build does not know, is refused rather than read as rounds.
TEST_F(ItemRoundTripTest, ADurationWithoutAKnownUnitIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(source.is_open()) << "cannot read the real items file";
	const nlohmann::json real = nlohmann::json::parse(source);

	nlohmann::json missing = real;
	missing.at("invisibility_potion").erase("durationUnit");
	std::ofstream(roundTrip) << missing.dump(4);
	ItemRegistry withoutUnit{};
	EXPECT_ANY_THROW(withoutUnit.load(roundTrip.string()));

	nlohmann::json unknown = real;
	unknown.at("scroll_sleep")["durationUnit"] = "hours";
	std::ofstream(roundTrip) << unknown.dump(4);
	ItemRegistry unknownUnit{};
	EXPECT_ANY_THROW(unknownUnit.load(roundTrip.string()));
}
