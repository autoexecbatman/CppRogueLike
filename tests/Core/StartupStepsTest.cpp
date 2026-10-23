// file: StartupStepsTest.cpp
// Startup runs its named steps in order, logs each by name, and stops at the first step
// that throws, reporting that step and its reason.
//
// What it is for. main's loaders threw past main, so a refused content file exited with
// nothing printed on native and a generic line on the web (issues/0328). main.cpp is not
// in this binary, so the steps and their reporting live in StartupSteps, which is.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=StartupSteps*

#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/StartupSteps.h"

namespace
{
// The order the steps ran in, by name.
struct RunOrder
{
	std::vector<std::string> names{};
};

// A step that records its name when it runs.
StartupStep recording_step(const std::string& name, RunOrder& order)
{
	const auto record_the_run = [name, &order]()
	{
		order.names.push_back(name);
	};
	return StartupStep{ name, record_the_run };
}
} // namespace

TEST(StartupStepsTest, EveryStepRunsInOrderAndNothingIsRefused)
{
	RunOrder order{};
	const std::vector<StartupStep> steps{
		recording_step("first", order),
		recording_step("second", order),
		recording_step("third", order),
	};
	std::ostringstream log{};

	const std::optional<StartupRefusal> refusal = run_startup_steps(steps, log);

	EXPECT_FALSE(refusal.has_value());
	EXPECT_EQ(
		order.names,
		(std::vector<std::string>{ "first", "second", "third" }));
}

TEST(StartupStepsTest, EachStepIsLoggedByNameBeforeItRuns)
{
	RunOrder order{};
	const std::vector<StartupStep> steps{
		recording_step("Loading tile config", order),
		recording_step("Loading spells", order)
	};
	std::ostringstream log{};

	[[maybe_unused]] const std::optional<StartupRefusal> refusal =
		run_startup_steps(steps, log);

	EXPECT_EQ(
		log.str(),
		"STARTUP: Loading tile config\nSTARTUP: Loading spells\n");
}

TEST(StartupStepsTest, AThrowingStepStopsStartupAndIsNamedWithItsReason)
{
	RunOrder order{};
	const auto refuse_the_data = []()
	{
		throw std::runtime_error("key 'tiles' not found");
	};
	const std::vector<StartupStep> steps{
		recording_step("first", order),
		StartupStep{ "Loading tile config", refuse_the_data },
		recording_step("third", order),
	};
	std::ostringstream log{};

	const std::optional<StartupRefusal> refusal = run_startup_steps(steps, log);

	ASSERT_TRUE(refusal.has_value()) << "a step that threw was not reported";
	EXPECT_EQ(refusal->step, "Loading tile config");
	EXPECT_EQ(refusal->reason, "key 'tiles' not found");
	EXPECT_EQ(order.names, (std::vector<std::string>{ "first" }))
		<< "a step after the refusal still ran";
	EXPECT_EQ(
		log.str(),
		"STARTUP: first\nSTARTUP: Loading tile config\nSTARTUP: Loading tile config failed: key 'tiles' not found\n");
}

TEST(StartupStepsTest, TheRefusalLineNamesTheStepAndTheReason)
{
	const StartupRefusal refusal{
		"Loading tile config",
		"key 'tiles' not found",
	};

	EXPECT_EQ(
		startup_refusal_line(refusal),
		"Could not start: Loading tile config failed: key 'tiles' not found");
}

// Only a std::exception is a refusal; anything else is a fault, and escapes to end the program.
TEST(StartupStepsTest, AThrowThatIsNotAStdExceptionEscapes)
{
	const auto throw_a_number = []()
	{
		throw 7;
	};
	const std::vector<StartupStep> steps{
		StartupStep{
			"Throwing a number",
			throw_a_number,
		},
	};
	std::ostringstream log{};

	EXPECT_THROW((void)run_startup_steps(steps, log), int)
		<< "a fault was swallowed as though it were a refusal";
}
