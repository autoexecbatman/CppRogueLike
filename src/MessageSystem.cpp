#include <iostream>
#include <string>

#include "Gui.h"
#include "LogMessage.h"
#include "MessageSystem.h"

MessageSystem::MessageSystem() = default;

void MessageSystem::begin_run()
{
	run = RunState{};
}

void MessageSystem::message(
	ColorPairId color,
	std::string_view text,
	MessageCompletion completion)
{
	// Store message in system
	run.messageToDisplay = text;
	run.messageColor = color;

	// Always append the message part to attackMessageParts
	run.attackMessageParts.push_back(LogMessage{ color, std::string(text) });

	// A finished message closes the entry; a continued one waits for its parts.
	if (completion == MessageCompletion::FINISHED)
	{
		// Add the entire composed message parts to attackMessagesWhole
		run.attackMessagesWhole.push_back(run.attackMessageParts);

		// Clear attackMessageParts for the next message
		run.attackMessageParts.clear();
	}

	log("Stored message: '" + run.messageToDisplay + "'");
	log(std::string{ "Stored message color: " } +
		std::string{ color_pair_name(run.messageColor) });
}

void MessageSystem::append_message_part(
	ColorPairId color,
	std::string_view text)
{
	run.attackMessageParts.push_back({ color, std::string(text) });
}

void MessageSystem::finalize_message()
{
	if (!run.attackMessageParts.empty())
	{
		run.attackMessagesWhole.push_back(run.attackMessageParts);
		run.attackMessageParts.clear();
	}
}

void MessageSystem::transfer_messages_to_gui(Gui& gui)
{
	for (const auto& message : run.attackMessagesWhole)
	{
		gui.add_display_message(message);
	}
	run.attackMessagesWhole.clear();
}

void MessageSystem::log(std::string_view message) const
{
	if (debugMode)
	{
		std::clog << message << "\n";
		std::cout << message << "\n";
	}
}

void MessageSystem::display_debug_messages() const noexcept
{
	// Not yet ported from curses to Raylib; log() already streams to std::clog in debug mode.
	log("display_debug_messages: debug viewer not yet implemented for Raylib");
}
