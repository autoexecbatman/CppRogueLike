#pragma once

// file: FailedParts.h
//
// Writes the failed parts gtest recorded to stderr, one "file:line: message"
// each. A death-test child forwards no gtest events, so a failure it records
// is printed by nothing else; what this writes is what the parent's failure
// shows as the child's text.
//
// Usage:
//
//   const int written = write_failed_parts(*test->result());
//   // -> 0 for a result with no failure, and nothing written
//   write_failed_parts_of_the_run();
//   // every failed part of every test the run held, from its own results
//
// Neither flushes; a caller about to leave flushes stderr itself.

#include <cstdio>

#include <gtest/gtest.h>

// Writes each failed part of one test's result and answers how many it wrote.
// A part recorded without a location, as gtest records a crash, names
// "(no file)".
//
// Example:
//   ADD_FAILURE() << "late"; write_failed_parts(*test->result());
//   // -> 1, having written "<its file>:<its line>: Failed\nlate"
//   write_failed_parts(*passingTest->result()); // -> 0, nothing written
inline int write_failed_parts(const ::testing::TestResult& result)
{
	int written = 0;
	for (int partIndex = 0; partIndex < result.total_part_count(); ++partIndex)
	{
		const ::testing::TestPartResult& part =
			result.GetTestPartResult(partIndex);
		if (!part.failed())
		{
			continue;
		}
		std::fprintf(
			stderr,
			"%s:%d: %s\n",
			part.file_name() != nullptr ? part.file_name() : "(no file)",
			part.line_number(),
			part.message());
		++written;
	}
	return written;
}

// Writes the failed parts of every test in the run, in the order it ran
// them, and answers how many it wrote.
//
// Example, in a child whose one test crashed:
//   write_failed_parts_of_the_run();
//   // -> 1, having written "(no file):-1: SEH exception with code
//   //    0xc0000005 thrown in the test body." and its stack trace
inline int write_failed_parts_of_the_run()
{
	const ::testing::UnitTest& run = *::testing::UnitTest::GetInstance();
	int written = 0;
	const int suiteCount = run.total_test_suite_count();
	for (int suiteIndex = 0; suiteIndex < suiteCount; ++suiteIndex)
	{
		const ::testing::TestSuite& suite = *run.GetTestSuite(suiteIndex);
		const int testCount = suite.total_test_count();
		for (int testIndex = 0; testIndex < testCount; ++testIndex)
		{
			const ::testing::TestInfo& test = *suite.GetTestInfo(testIndex);
			written += write_failed_parts(*test.result());
		}
	}
	return written;
}

// end of file: FailedParts.h
