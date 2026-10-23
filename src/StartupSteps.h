#pragma once

#include <functional>
#include <optional>
#include <ostream>
#include <span>
#include <string>

// The program's startup, as a list of named steps run in order. A step that throws
// stops the startup, and what it threw is reported to the player by the step's name,
// on the console and the log, and on the web page too.
//
// Usage, as main does it:
//
//   const auto load_tiles = [&game]() { game.tileConfig.load(Paths::TILE_CONFIG); };
//   const std::vector<StartupStep> steps{
//       { "Loading tile config", load_tiles },      // the name is the log line
//   };
//   const std::optional<StartupRefusal> refusal = run_startup_steps(steps, std::clog);
//   if (refusal)
//   {
//       report_startup_refusal(*refusal);           // stderr, and the page on the web
//       return 1;                                   // the game does not run
//   }

// One step of startup: what the log calls it, and the work it does.
struct StartupStep
{
	// The log line's text, such as "Loading tile config".
	std::string name{};
	// The step's work; a std::exception from it is a refusal.
	std::function<void()> run{};
};

// The step that refused, and the reason its exception gave.
struct StartupRefusal
{
	// The name of the step that threw.
	std::string step{};
	// What its exception's what() said.
	std::string reason{};
};

// Runs the steps in order, writing "STARTUP: <name>" to the log before each. A step that
// throws a std::exception stops the startup: its failure is logged and returned, and no
// later step runs. A throw of anything else is not caught and ends the program.
//
// Example:
//   run_startup_steps(steps, log);   // all ran -> nullopt, log "STARTUP: first\n..."
//   run_startup_steps(steps, log);   // second threw "key 'tiles' not found"
//                                    // -> { "Loading tile config", "key 'tiles' not found" }
[[nodiscard]] std::optional<StartupRefusal> run_startup_steps(
	std::span<const StartupStep> steps,
	std::ostream& log);

// The line a refusal is reported with, naming the step and its reason.
//
// Example:
//   startup_refusal_line({ "Loading tile config", "key 'tiles' not found" });
//   // -> "Could not start: Loading tile config failed: key 'tiles' not found"
[[nodiscard]] std::string startup_refusal_line(const StartupRefusal& refusal);

// Shows the refusal's line to the player: on stderr, and on the web in the page's loading
// overlay, since no window exists yet when a content loader refuses.
//
// Example:
//   report_startup_refusal(refusal);   // stderr: "Could not start: Loading tile config failed: ..."
void report_startup_refusal(const StartupRefusal& refusal);
