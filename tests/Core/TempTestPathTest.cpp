// file: TempTestPathTest.cpp
// temp_test_path names a file after the process and the running test, so two
// test_exe runs, or two tests in one run, never share one.
//
// The typed case pins the slash: GoogleTest names a typed suite
// "Suite/0", and a slash left in would put the file in a directory nobody made.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TempTestPath*

#include <gtest/gtest.h>

#include <format>
#include <process.h>

#include "tests/mocks/TempTestPath.h"

namespace CppRL
{

	TEST(TempTestPathTest, TheNameCarriesTheProcessTheSuiteAndTheTest)
	{
		EXPECT_EQ(
			temp_test_path("items.json").filename().string(),
			std::format(
				"{}_TempTestPathTest_TheNameCarriesTheProcessTheSuiteAndTheTest_"
				"items.json",
				_getpid()));
	}

	template <typename Unused>
	class TempTestPathTypedTest : public ::testing::Test
	{
	};

	using OneType = ::testing::Types<int>;
	TYPED_TEST_SUITE(TempTestPathTypedTest, OneType);

	TYPED_TEST(
		TempTestPathTypedTest,
		ASlashInATypedSuiteNameBecomesAnUnderscore)
	{
		EXPECT_EQ(
			temp_test_path("items.json").filename().string(),
			std::format(
				"{}_TempTestPathTypedTest_0_"
				"ASlashInATypedSuiteNameBecomesAnUnderscore_items.json",
				_getpid()));
	}

} // namespace CppRL
