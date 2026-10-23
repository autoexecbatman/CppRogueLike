// file: TileConfigLoadTest.cpp
//
// Checks that the tile config refuses a tile or autotile record missing one of its
// fields, naming the field. Those records were read through a const json's operator[],
// which is undefined for a missing key: an abort in a Debug build, and in the Release
// build players run, whatever memory sat past the end of the object.
//
// It also checks that a config missing any of the four sections the loader reads is
// refused, naming the section.
//
// What it does not check: the fields of a tile type, whose parser has its own tests in
// TileDefinitionTest, and what a refusal leaves loaded - TileConfig::load fills its
// members section by section, so an earlier section stays.
//
// Each test damages a copy of the real file in its own temporary file, named after the
// test, so no two tests share one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=TileConfig*:EverySectionTheLoaderReads*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/Paths.h"
#include "src/TileConfig.h"

class TileConfigLoadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// A parameterised case's name holds a slash, which would make the file a path.
		std::string testName =
			::testing::UnitTest::GetInstance()->current_test_info()->name();
		std::ranges::replace(testName, '/', '_');
		scratch = std::filesystem::temp_directory_path() /
			("tile_config_" + testName + ".json");
	}

	void TearDown() override { std::filesystem::remove(scratch); }

	static nlohmann::json read_real_config()
	{
		std::ifstream source(Paths::resolve(Paths::TILE_CONFIG));
		EXPECT_TRUE(source.is_open()) << "cannot read " << Paths::TILE_CONFIG;
		return nlohmann::json::parse(source);
	}

	// Removes `field` from the first record of `section`, writes the result, and returns
	// what loading it threw.
	std::string refusal_without(
		const std::string& section,
		const std::string& field)
	{
		nlohmann::json document = read_real_config();
		document.at(section).begin()->erase(field);
		{
			std::ofstream out(scratch);
			out << document.dump(2);
		}

		try
		{
			TileConfig config{};
			config.load(scratch.string());
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << section << " without " << field << " was not refused";
		return {};
	}

	// Removes the whole of `section`, writes the result, and returns what loading it threw.
	std::string refusal_without_section(const std::string& section)
	{
		nlohmann::json document = read_real_config();
		EXPECT_TRUE(document.contains(section))
			<< "the shipped config has no " << section
			<< ", so erasing it proves nothing";
		document.erase(section);
		{
			std::ofstream out(scratch);
			out << document.dump(2);
		}

		try
		{
			TileConfig config{};
			config.load(scratch.string());
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << "a config without " << section << " was not refused";
		return {};
	}

	std::filesystem::path scratch{};
};

// A tile missing any of its three fields is refused by name.
TEST_F(TileConfigLoadTest, ATileMissingAFieldIsRefusedByName)
{
	for (const char* field : { "sheet", "col", "row" })
	{
		const std::string refusal = refusal_without("tiles", field);

		EXPECT_NE(refusal.find(field), std::string::npos)
			<< "refusal does not name " << field << ": " << refusal;
	}
}

// Both autotile sections share a shape: a sheet and the origin of the block of tiles.
TEST_F(TileConfigLoadTest, AnAutotileGroupMissingAFieldIsRefusedByName)
{
	for (const char* section : { "autotile_groups", "wall_autotile_groups" })
	{
		for (const char* field : { "sheet", "origin_col", "origin_row" })
		{
			const std::string refusal = refusal_without(section, field);

			EXPECT_NE(refusal.find(field), std::string::npos)
				<< section << ": refusal does not name " << field << ": "
				<< refusal;
		}
	}
}

// The same file, once per section the loader reads: every one is required.
class TileConfigSectionTest :
	public TileConfigLoadTest,
	public ::testing::WithParamInterface<std::string_view>
{
};

TEST_P(TileConfigSectionTest, AConfigMissingTheSectionIsRefusedByName)
{
	const std::string section{ GetParam() };

	const std::string refusal = refusal_without_section(section);

	EXPECT_NE(refusal.find(section), std::string::npos)
		<< "refusal does not name " << section << ": " << refusal;
}

// Each case is named for its section, so a failure says which one without reading the output.
const auto name_for_section =
	[](const ::testing::TestParamInfo<std::string_view>& info)
{
	return std::string{ info.param };
};

INSTANTIATE_TEST_SUITE_P(
	EverySectionTheLoaderReads,
	TileConfigSectionTest,
	::testing::Values(
		"tiles",
		"autotile_groups",
		"wall_autotile_groups",
		"tile_types"),
	name_for_section);
