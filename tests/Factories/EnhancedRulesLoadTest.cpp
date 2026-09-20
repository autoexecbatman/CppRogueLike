// Checks that the enhanced spawn rules refuse to load quietly.
//
// What it is for. enhanced_rules.json decides which items can spawn with a prefix or
// suffix and at what depth. It is a second schema, separate from items.json, loaded by
// its own function - ItemRoundTripTest says in its own header that it does not cover it.
// Every failure in this loader used to be silent: an unreadable file returned, a missing
// level_maximum defaulted, and any enhancement_category that was not exactly "weapon"
// became ARMOR. None of those states is legitimate - main.cpp loads the file
// unconditionally, all four records carry all seven fields, and only "weapon" and
// "armor" appear.
//
// What it deliberately does not check. Not that the rules are balanced or that the item
// pools name items that exist, only that a file which cannot be honoured is refused.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EnhancedRulesLoadTest.*

#include "src/ItemRegistry.h"
#include "src/Paths.h"
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

class EnhancedRulesLoadTest : public ::testing::Test
{
protected:
	std::filesystem::path damaged;
	ItemRegistry items{};

	void SetUp() override
	{
		damaged = std::filesystem::temp_directory_path() / "enhanced_rules_damaged.json";
	}

	void TearDown() override
	{
		std::filesystem::remove(damaged);
	}

	// Writes the real rules to the temporary path with one edit applied to the first
	// record, and returns what loading it threw.
	std::string load_with_first_rule(const std::string& field, const nlohmann::json& value)
	{
		std::ifstream source(Paths::resolve(Paths::ENHANCED_RULES));
		EXPECT_TRUE(source.is_open()) << "cannot read the real enhanced rules";
		nlohmann::json root = nlohmann::json::parse(source);
		EXPECT_FALSE(root.empty()) << "no rules to damage";
		if (value.is_null())
		{
			root[0].erase(field);
		}
		else
		{
			root[0][field] = value;
		}

		std::ofstream out(damaged);
		out << root.dump(2);
		out.close();

		try
		{
			items.load_enhanced_rules(damaged.string());
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << "loading a rule with " << field << " damaged threw nothing";
		return {};
	}
};

TEST_F(EnhancedRulesLoadTest, AFileThatWillNotOpenIsRefused)
{
	// A missing rules file leaves no enhanced items in the world and used to say
	// nothing, which is indistinguishable from a dungeon that simply rolled none.
	try
	{
		items.load_enhanced_rules("data/content/no_such_rules.json");
		ADD_FAILURE() << "loading a path that does not exist threw nothing";
	}
	catch (const std::runtime_error& refusal)
	{
		EXPECT_NE(std::string(refusal.what()).find("no_such_rules.json"), std::string::npos)
			<< "the refusal must name the path it could not open: " << refusal.what();
	}
}

TEST_F(EnhancedRulesLoadTest, AnUnknownEnhancementCategoryIsRefused)
{
	// The old ternary read "weapon" and called everything else ARMOR, so a typo became
	// a silently different rule rather than an error.
	const std::string refusal = load_with_first_rule("enhancement_category", "wepaon");

	EXPECT_NE(refusal.find("wepaon"), std::string::npos)
		<< "the refusal must name the value it did not recognise: " << refusal;
}

TEST_F(EnhancedRulesLoadTest, ARuleMissingAFieldIsRefused)
{
	// level_maximum was the one field read with a default, and every record carries it.
	const std::string refusal = load_with_first_rule("level_maximum", nlohmann::json{});

	EXPECT_FALSE(refusal.empty()) << "a rule missing level_maximum must be refused";
}

TEST_F(EnhancedRulesLoadTest, TheRealFileLoadsWithoutThrowing)
{
	// Paired with the refusals: they must be about the damage rather than about the
	// loader, or they would pass while rejecting everything.
	EXPECT_NO_THROW(items.load_enhanced_rules(Paths::ENHANCED_RULES));
	EXPECT_FALSE(items.get_enhanced_rules().empty());
}

TEST_F(EnhancedRulesLoadTest, AMissingItemFileIsRefused)
{
	// The same hole, one function up: a missing items file is refused the way every
	// other loader refuses its own.
	try
	{
		items.load("data/content/no_such_items.json");
		ADD_FAILURE() << "loading a path that does not exist threw nothing";
	}
	catch (const std::runtime_error& refusal)
	{
		EXPECT_NE(std::string(refusal.what()).find("no_such_items.json"), std::string::npos)
			<< "the refusal must name the path it could not open: " << refusal.what();
	}
}
