// file: PrefabLibraryLoadTest.cpp
//
// Checks that the prefab library refuses data it cannot honour instead of loading
// something plausible. The parser is the only schema prefabs.json has: a record missing
// its weight used to load at weight 1 and depth 1 to 99, and a file that did not open left
// the game with no prefabs at all, which no player would notice and no test could see.
// The palette the prefabs' symbols resolve through is PaletteTest's.
//
// Each test damages a copy of the real file in its own temporary file, named after the
// test, so no two tests share one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=PrefabLibraryLoadTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "src/Paths.h"
#include "src/PrefabLibrary.h"

class PrefabLibraryLoadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		const std::string testName =
			::testing::UnitTest::GetInstance()->current_test_info()->name();
		scratch = std::filesystem::temp_directory_path() /
			("prefab_library_" + testName + ".json");
	}

	void TearDown() override { std::filesystem::remove(scratch); }

	static nlohmann::json read_json(std::string_view path)
	{
		std::ifstream source(Paths::resolve(path));
		EXPECT_TRUE(source.is_open()) << "cannot read " << path;
		return nlohmann::json::parse(source);
	}

	void write_scratch(const nlohmann::json& document) const
	{
		std::ofstream out(scratch);
		out << document.dump(2);
	}

	// What the call threw, or a failure if it threw nothing.
	template <typename LoadCall>
	std::string refusal_of(LoadCall load_call)
	{
		try
		{
			load_call();
		}
		catch (const std::exception& refusal)
		{
			return refusal.what();
		}
		ADD_FAILURE() << "the call was not refused";
		return {};
	}

	std::filesystem::path scratch{};
	PrefabLibrary library{};
};

// The real file loads whole: one prefab per record.
TEST_F(PrefabLibraryLoadTest, TheRealFileLoadsWhole)
{
	const nlohmann::json prefabs = read_json(Paths::PREFABS);

	library.load(Paths::PREFABS);

	EXPECT_EQ(library.all().size(), prefabs.at("prefabs").size());
}

// Every required prefab field, removed from the first record, is named in the refusal.
TEST_F(PrefabLibraryLoadTest, APrefabMissingAFieldIsRefusedByName)
{
	for (
		const char* field :
		{ "name", "depth_min", "depth_max", "weight", "rows" })
	{
		nlohmann::json document = read_json(Paths::PREFABS);
		document.at("prefabs").at(0).erase(field);
		write_scratch(document);

		const auto load_scratch_prefabs = [&]()
		{
			library.load(scratch.string());
		};
		const std::string refusal = refusal_of(load_scratch_prefabs);

		EXPECT_NE(refusal.find(field), std::string::npos)
			<< "refusal does not name " << field << ": " << refusal;
	}
}

// A prefab with no rows would stamp nothing, so it is refused by name.
TEST_F(PrefabLibraryLoadTest, APrefabWithNoRowsIsRefusedByName)
{
	nlohmann::json document = read_json(Paths::PREFABS);
	document.at("prefabs").at(0)["rows"] = nlohmann::json::array();
	const std::string name =
		document.at("prefabs").at(0).at("name").get<std::string>();
	write_scratch(document);

	const auto load_scratch_prefabs = [&]()
	{
		library.load(scratch.string());
	};
	const std::string refusal = refusal_of(load_scratch_prefabs);

	EXPECT_NE(refusal.find(name), std::string::npos) << refusal;
}

// A path that does not open is refused, not read as an empty library.
TEST_F(PrefabLibraryLoadTest, AMissingPrefabFileIsRefused)
{
	const auto load_scratch_prefabs = [&]()
	{
		library.load(scratch.string());
	};
	const std::string refusal = refusal_of(load_scratch_prefabs);

	EXPECT_NE(refusal.find("cannot open"), std::string::npos) << refusal;
}

// A file without its "prefabs" list is refused by name.
TEST_F(PrefabLibraryLoadTest, APrefabFileWithoutItsListIsRefusedByName)
{
	write_scratch(nlohmann::json{ { "rooms", nlohmann::json::array() } });

	const auto load_scratch_prefabs = [&]()
	{
		library.load(scratch.string());
	};
	const std::string refusal = refusal_of(load_scratch_prefabs);

	EXPECT_NE(refusal.find("prefabs"), std::string::npos) << refusal;
}

// Text that is not JSON is refused.
TEST_F(PrefabLibraryLoadTest, AMalformedPrefabFileIsRefused)
{
	{
		std::ofstream out(scratch);
		out << "{ \"prefabs\": [ ";
	}

	const auto load_scratch_prefabs = [&]()
	{
		library.load(scratch.string());
	};
	const std::string refusal = refusal_of(load_scratch_prefabs);

	EXPECT_FALSE(refusal.empty());
}

// A refused file leaves the prefabs already loaded in place.
TEST_F(PrefabLibraryLoadTest, ARefusedFileLeavesTheLoadedPrefabs)
{
	library.load(Paths::PREFABS);
	const size_t loaded = library.all().size();
	ASSERT_GT(loaded, 0u);
	nlohmann::json document = read_json(Paths::PREFABS);
	document.at("prefabs").at(1).erase("weight");
	write_scratch(document);

	const auto load_scratch_prefabs = [&]()
	{
		library.load(scratch.string());
	};
	[[maybe_unused]] const std::string refusal =
		refusal_of(load_scratch_prefabs);

	EXPECT_EQ(library.all().size(), loaded);
}

// A path that cannot be written - here a directory - is refused, naming the path, instead
// of reporting a save that never reached the disk.
TEST_F(PrefabLibraryLoadTest, ASaveThatCannotBeWrittenIsRefused)
{
	library.load(Paths::PREFABS);
	const std::filesystem::path directory =
		std::filesystem::temp_directory_path();
	const auto save_to_directory = [&]()
	{
		library.save(directory.string());
	};

	const std::string refusal = refusal_of(save_to_directory);

	EXPECT_NE(refusal.find("cannot write"), std::string::npos) << refusal;
}

// What save writes, load reads back as the same prefabs.
TEST_F(PrefabLibraryLoadTest, SavedPrefabsLoadBackTheSame)
{
	library.load(Paths::PREFABS);
	library.save(scratch.string());
	PrefabLibrary reloaded{};

	reloaded.load(scratch.string());

	ASSERT_EQ(reloaded.all().size(), library.all().size());
	for (size_t index = 0; index < library.all().size(); ++index)
	{
		EXPECT_EQ(reloaded.all()[index].name, library.all()[index].name);
		EXPECT_EQ(
			reloaded.all()[index].depthMin,
			library.all()[index].depthMin);
		EXPECT_EQ(
			reloaded.all()[index].depthMax,
			library.all()[index].depthMax);
		EXPECT_EQ(reloaded.all()[index].weight, library.all()[index].weight);
		EXPECT_EQ(reloaded.all()[index].rows, library.all()[index].rows);
	}
}
