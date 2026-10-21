#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "StartupSteps.h"

std::optional<StartupRefusal> run_startup_steps(std::span<const StartupStep> steps, std::ostream& log)
{
	for (const StartupStep& step : steps)
	{
		log << "STARTUP: " << step.name << '\n'
			<< std::flush;
		// A std::exception is bad data refusing the step; anything else is a fault and escapes.
		try
		{
			step.run();
		}
		catch (const std::exception& refusal)
		{
			log << "STARTUP: " << step.name << " failed: " << refusal.what() << '\n'
				<< std::flush;
			return StartupRefusal{ step.name, refusal.what() };
		}
	}
	return std::nullopt;
}

std::string startup_refusal_line(const StartupRefusal& refusal)
{
	return std::format("Could not start: {} failed: {}", refusal.step, refusal.reason);
}

void report_startup_refusal(const StartupRefusal& refusal)
{
	const std::string line = startup_refusal_line(refusal);
	std::cerr << line << std::endl;

#ifdef __EMSCRIPTEN__
	// No window exists yet, so the page's loading overlay carries the line; textContent, since it comes from data.
	EM_ASM(
		{
			var overlay = document.getElementById('loading-overlay');
			var status = document.getElementById('status');
			if (overlay)
			{
				overlay.style.display = 'flex';
			}
			if (status)
			{
				status.textContent = UTF8ToString($0);
			}
			// The runtime clears the status once main returns; later status text goes to the console instead.
			Module.setStatus = function(text)
			{
				if (text)
				{
					console.error('[after refusal] ' + text);
				}
			};
		},
		line.c_str());
#endif
}
