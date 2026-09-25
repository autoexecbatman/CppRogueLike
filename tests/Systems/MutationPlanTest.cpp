// file: MutationPlanTest.cpp
// Every mutation plan in tests/ points at code that exists and at suites that exist.
//
// What it is for. A plan is a JSON file of edits, each naming a fragment of source to
// replace and a gtest filter to run afterwards. Both halves fail silently. `mutate.py`
// skips a mutation whose fragment it cannot find and still exits 0, so a plan whose
// anchors drifted reports nothing wrong while testing nothing; and GoogleTest accepts a
// filter naming a suite that does not exist, matching no tests and saying so to nobody.
// `room_shape_mutations.json` named `TreasureRoomTest`, which has never existed - the
// fixture is `TreasureRoomFixture` - so a third of that plan's intended coverage never
// ran.
//
// This is the census that used to be done by hand before every commit. Doing it here
// means a plan edited out from under itself fails on the next suite run instead of on
// the next time somebody remembers.
//
// What it does not check: whether a mutation is worth making, or whether the suite it
// names would actually catch it. Only `mutate.py` answers that, by running it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MutationPlanTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/Paths.h"

namespace
{

using json = nlohmann::json;

// Every plan under tests/, found rather than listed, so a new one is covered by being
// written.
std::vector<std::filesystem::path> every_plan()
{
	std::vector<std::filesystem::path> plans;
	const std::filesystem::path root = Paths::resolve("tests");
	if (!std::filesystem::is_directory(root))
	{
		return plans;
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
	{
		const std::string name = entry.path().filename().string();
		if (entry.is_regular_file() && name.contains("mutations") && entry.path().extension() == ".json")
		{
			plans.push_back(entry.path());
		}
	}
	return plans;
}

// The file as mutate.py reads it: carriage returns folded away, so an anchor written with
// plain newlines matches a file the working tree keeps in CRLF.
std::string source_as_the_tool_reads_it(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	const std::string raw{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	std::string folded;
	folded.reserve(raw.size());
	for (size_t index = 0; index < raw.size(); ++index)
	{
		if (raw[index] == '\r' && index + 1 < raw.size() && raw[index + 1] == '\n')
		{
			continue;
		}
		folded += raw[index];
	}
	return folded;
}

// The suite names this binary actually registers.
std::set<std::string> registered_suites()
{
	std::set<std::string> names;
	const ::testing::UnitTest& unitTest = *::testing::UnitTest::GetInstance();
	for (int index = 0; index < unitTest.total_test_suite_count(); ++index)
	{
		names.insert(unitTest.GetTestSuite(index)->name());
	}
	return names;
}

} // namespace

// A fragment that is no longer in its file is a mutation that never runs, and mutate.py
// reports that as a skip while still exiting 0.
TEST(MutationPlanTest, EveryAnchorAppearsExactlyOnceInItsFile)
{
	const std::vector<std::filesystem::path> plans = every_plan();
	ASSERT_FALSE(plans.empty()) << "no plans found; the case measures nothing";

	for (const std::filesystem::path& planPath : plans)
	{
		std::ifstream planFile(planPath);
		ASSERT_TRUE(planFile.is_open()) << planPath.string();
		json plan;
		planFile >> plan;

		for (const auto& mutation : plan.at("mutations"))
		{
			const std::string file = mutation.at("file").get<std::string>();
			const std::string anchor = mutation.at("original").get<std::string>();
			const std::string source = source_as_the_tool_reads_it(Paths::resolve(file));

			size_t occurrences = 0;
			for (size_t at = source.find(anchor); at != std::string::npos; at = source.find(anchor, at + 1))
			{
				++occurrences;
			}
			EXPECT_EQ(occurrences, 1u)
				<< planPath.filename().string() << " -> " << file
				<< ": the fragment starting \"" << anchor.substr(0, 60) << "\" appears "
				<< occurrences << " times";
		}
	}
}

// A filter naming a suite that does not exist runs nothing and says nothing.
TEST(MutationPlanTest, EveryFilterNamesASuiteThatExists)
{
	const std::set<std::string> suites = registered_suites();
	ASSERT_FALSE(suites.empty());

	for (const std::filesystem::path& planPath : every_plan())
	{
		std::ifstream planFile(planPath);
		ASSERT_TRUE(planFile.is_open()) << planPath.string();
		json plan;
		planFile >> plan;

		const std::string command = plan.at("test_command").back().get<std::string>();
		const std::string flag = "--gtest_filter=";
		const size_t at = command.find(flag);
		if (at == std::string::npos)
		{
			continue;
		}

		std::string filter = command.substr(at + flag.size());
		filter = filter.substr(0, filter.find_first_of(" \t"));

		size_t start = 0;
		while (start <= filter.size())
		{
			const size_t stop = filter.find(':', start);
			const std::string pattern = filter.substr(start, stop == std::string::npos ? std::string::npos : stop - start);
			start = stop == std::string::npos ? filter.size() + 1 : stop + 1;

			if (pattern.empty() || pattern.front() == '-')
			{
				continue;
			}
			std::string suite = pattern.substr(0, pattern.find('.'));
			while (!suite.empty() && suite.back() == '*')
			{
				suite.pop_back();
			}
			if (suite.empty())
			{
				continue;
			}

			// A pattern may name a fixture's prefix rather than the whole suite.
			const auto names_this_suite = [&suite](const std::string& registered)
			{
				return registered.starts_with(suite);
			};
			const bool known = suites.contains(suite) || std::ranges::any_of(suites, names_this_suite);
			EXPECT_TRUE(known)
				<< planPath.filename().string() << ": the filter names '" << pattern
				<< "', which matches no test suite in this binary";
		}
	}
}
