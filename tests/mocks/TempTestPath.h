#pragma once

// file: TempTestPath.h
// Where a test puts a file or directory it makes and removes: the system's
// temporary directory, under a name made of this process's id, the running
// test's suite and name, and the name the test gives. Two test_exe runs at
// once, or two tests in one run, then never read or delete each other's files,
// and no test types a prefix to keep its names apart.
//
// Example, in TempTestPathTest.TheNameCarriesTheProcessTheSuiteAndTheTest
// in the run whose process id is 45784:
//   temp_test_path("items.json");
//   // -> %TEMP%\ and, as one name, 45784_TempTestPathTest_
//   //    TheNameCarriesTheProcessTheSuiteAndTheTest_items.json
//   temp_test_path("probe") / "data";
//   // -> a directory tree of the test's own; the test creates and removes it

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <process.h>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace CppRL
{

	// The process id, the running test's suite and name, then the name the
	// test gives, in the temporary directory. Every "/" becomes "_", since a
	// typed or parameterised test's name holds one, so the result is one file
	// name rather than a path through directories nobody made. Asserts when
	// no test is running.
	//
	// Example, in TempTestPathTypedTest/0, the run whose process id is 45784:
	//   temp_test_path("items.json");
	//   // -> %TEMP%\ and, as one name, 45784_TempTestPathTypedTest_0_
	//   //    ASlashInATypedSuiteNameBecomesAnUnderscore_items.json
	//   temp_test_path("items.json"); // from main, outside any test
	//   // -> aborts: "temp_test_path called outside a running test"
	[[nodiscard]] inline std::filesystem::path temp_test_path(
		std::string_view name)
	{
		const ::testing::TestInfo* const running =
			::testing::UnitTest::GetInstance()->current_test_info();
		assert(running && "temp_test_path called outside a running test");
		std::string fileName = std::format(
			"{}_{}_{}_{}",
			_getpid(),
			running->test_suite_name(),
			running->name(),
			name);
		std::ranges::replace(fileName, '/', '_');
		return std::filesystem::temp_directory_path() / fileName;
	}

} // namespace CppRL
