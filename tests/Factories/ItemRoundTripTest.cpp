// Checks that saving items.json and reading it back preserves what was authored.
//
// What it is for. items.json is the file this project has already lost once: the data
// used snake_case keys while the parser read camelCase, every field silently took its
// default, and create_random_of_category returned nullptr forever because baseWeight came
// back 0. Nothing threw and nothing logged. parse_item_entry still reads all thirty-four
// of its keys with j.value(), so that failure is still available - every field here can
// vanish without a sound, which makes this the file where a round-trip earns the most.
//
// What it deliberately does not check. Not that values are correct, only that they
// survive. Not the enhanced spawn rules, which load from a separate file through
// load_enhanced_rules and have their own schema.
//
// A trap this file had to work around, worth knowing before editing it. ItemParams holds
// name and category as std::string_view pointing into the ItemEntry that owns them, and
// load() clears the registry. A params copied by value before a reload therefore has two
// dangling views afterwards, and comparing them reads freed memory. The owned strings are
// copied out separately below.
//
// The oracle is parse_item_entry, the only schema this data has: every key the parser
// reads is a key the encoder must write. Expected values come from the entry loaded
// before the save, never from what the encoder produces.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemRoundTripTest.*

#include "src/Factories/ItemCreator.h"
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace
{

// An item as it stood before the save, with the two viewed strings owned so they survive
// the registry being cleared.
struct ItemSnapshot
{
	std::string name{};
	std::string category{};
	ItemParams params{};
};

}

class ItemRoundTripTest : public ::testing::Test
{
protected:
	std::filesystem::path roundTrip;

	void SetUp() override
	{
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
		roundTrip = std::filesystem::temp_directory_path() / "items_roundtrip.json";
	}

	void TearDown() override
	{
		// Leave the shared registry holding the real file, not the temporary one.
		ItemCreator::load("data/content/items.json");
		ItemCreator::load_enhanced_rules("data/content/enhanced_rules.json");
		std::filesystem::remove(roundTrip);
	}
};

// Every field ItemParams declares, named one at a time, so a lost key fails pointing at
// the field rather than at a count.
void expect_same_item(const std::string& key, const ItemSnapshot& before, const ItemParams& after)
{
	EXPECT_EQ(before.name, after.name) << key << ".name";
	EXPECT_EQ(before.category, after.category) << key << ".category";
	EXPECT_EQ(before.params.color, after.color) << key << ".color";
	EXPECT_EQ(before.params.itemClass, after.itemClass) << key << ".itemClass";
	EXPECT_EQ(before.params.value, after.value) << key << ".value";
	EXPECT_EQ(before.params.pickableType, after.pickableType) << key << ".pickableType";

	EXPECT_EQ(before.params.consumableAmount, after.consumableAmount) << key << ".consumableAmount";
	EXPECT_EQ(before.params.range, after.range) << key << ".range";
	EXPECT_EQ(before.params.damage, after.damage) << key << ".damage";
	EXPECT_EQ(before.params.confuseTurns, after.confuseTurns) << key << ".confuseTurns";
	EXPECT_EQ(before.params.duration, after.duration) << key << ".duration";

	EXPECT_EQ(before.params.effect, after.effect) << key << ".effect";
	EXPECT_EQ(before.params.effectBonus, after.effectBonus) << key << ".effectBonus";

	EXPECT_EQ(before.params.strBonus, after.strBonus) << key << ".strBonus";
	EXPECT_EQ(before.params.dexBonus, after.dexBonus) << key << ".dexBonus";
	EXPECT_EQ(before.params.conBonus, after.conBonus) << key << ".conBonus";
	EXPECT_EQ(before.params.intBonus, after.intBonus) << key << ".intBonus";
	EXPECT_EQ(before.params.wisBonus, after.wisBonus) << key << ".wisBonus";
	EXPECT_EQ(before.params.chaBonus, after.chaBonus) << key << ".chaBonus";
	EXPECT_EQ(before.params.isSetMode, after.isSetMode) << key << ".isSetMode";

	EXPECT_EQ(before.params.nutritionValue, after.nutritionValue) << key << ".nutritionValue";
	EXPECT_EQ(before.params.goldAmount, after.goldAmount) << key << ".goldAmount";
	EXPECT_EQ(before.params.acBonus, after.acBonus) << key << ".acBonus";

	EXPECT_EQ(before.params.ranged, after.ranged) << key << ".ranged";
	EXPECT_EQ(before.params.handRequirement, after.handRequirement) << key << ".handRequirement";
	EXPECT_EQ(before.params.weaponSize, after.weaponSize) << key << ".weaponSize";

	EXPECT_EQ(before.params.consumableEffect, after.consumableEffect) << key << ".consumableEffect";
	EXPECT_EQ(before.params.consumableBuffType, after.consumableBuffType) << key << ".consumableBuffType";
	EXPECT_EQ(before.params.targetMode, after.targetMode) << key << ".targetMode";
	EXPECT_EQ(before.params.scrollAnimation, after.scrollAnimation) << key << ".scrollAnimation";

	EXPECT_EQ(before.params.baseWeight, after.baseWeight) << key << ".baseWeight";
	EXPECT_EQ(before.params.levelMin, after.levelMin) << key << ".levelMin";
	EXPECT_EQ(before.params.levelMax, after.levelMax) << key << ".levelMax";
	EXPECT_FLOAT_EQ(before.params.levelScaling, after.levelScaling) << key << ".levelScaling";
}

TEST_F(ItemRoundTripTest, EveryPersistedFieldSurvivesASave)
{
	std::map<std::string, ItemSnapshot> before;
	for (const std::string& key : ItemCreator::get_all_keys())
	{
		const ItemParams& params = ItemCreator::get_params(key);
		before.emplace(key, ItemSnapshot{ std::string{ params.name }, std::string{ params.category }, params });
	}
	ASSERT_FALSE(before.empty()) << "no items loaded; the test would pass vacuously";

	ItemCreator::save(roundTrip.string());
	ItemCreator::load(roundTrip.string());

	for (const auto& [key, snapshot] : before)
	{
		expect_same_item(key, snapshot, ItemCreator::get_params(key));
	}
}

// Writes the real file to the temporary path with one key removed from one record, and
// returns what loading it threw. Fails the calling test if nothing was thrown.
std::string load_without_field(
	const std::filesystem::path& destination,
	const std::string& itemKey,
	const std::string& field)
{
	std::ifstream source(std::filesystem::path{ "data/content/items.json" });
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
		ItemCreator::load(destination.string());
	}
	catch (const std::runtime_error& refusal)
	{
		return refusal.what();
	}
	ADD_FAILURE() << "loading a record with no " << field << " threw nothing";
	return {};
}

// The encoder writes all thirty-four keys unconditionally and every record carries all
// thirty-four, so a record missing one is a corrupted or hand-edited file. Loading it
// must fail loudly: the alternative is the registry filling with items whose every value
// is a default, which is the failure this file exists for.
//
// Every field is tried rather than a chosen one, because the defect this replaces was a
// property of one line at a time - thirty-three correct reads and one silent default is
// exactly the state that produced the original bug.
TEST_F(ItemRoundTripTest, EveryFieldOfARecordIsRequired)
{
	std::ifstream source(std::filesystem::path{ "data/content/items.json" });
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
		const std::string refusal = load_without_field(roundTrip, "health_potion", field);

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
	EXPECT_NO_THROW(ItemCreator::load("data/content/items.json"));
	EXPECT_FALSE(ItemCreator::get_all_keys().empty());
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
	for (const std::string& key : ItemCreator::get_all_keys())
	{
		const ItemParams& params = ItemCreator::get_params(key);
		if (params.category.empty())
		{
			continue;
		}

		EXPECT_GT(params.baseWeight, 0)
			<< key << " is in category '" << params.category
			<< "' but has no spawn weight, so that category can never draw it";
	}
}
