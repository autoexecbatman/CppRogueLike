#include <iostream>
#include <string>

#include "../Gui/Gui.h"
#include "../Gui/LogMessage.h"
#include "MessageSystem.h"

MessageSystem::MessageSystem() = default;

void MessageSystem::message(int color, std::string_view text, bool isComplete)
{
    // Store message in system
    messageToDisplay = text;
    messageColor = color;

    // Always append the message part to attackMessageParts
    attackMessageParts.push_back(LogMessage{ color, std::string(text) });

    // If isComplete flag is set, consider the message to be finished
    if (isComplete)
    {
        // Add the entire composed message parts to attackMessagesWhole
        attackMessagesWhole.push_back(attackMessageParts);

        // Clear attackMessageParts for the next message
        attackMessageParts.clear();
    }

    log("Stored message: '" + messageToDisplay + "'");
    log("Stored message color: " + std::to_string(messageColor));
}

void MessageSystem::append_message_part(int color, std::string_view text)
{
    attackMessageParts.push_back({ color, std::string(text) });
}

void MessageSystem::finalize_message()
{
    if (!attackMessageParts.empty())
    {
        attackMessagesWhole.push_back(attackMessageParts);
        attackMessageParts.clear();
    }
}

void MessageSystem::transfer_messages_to_gui(Gui& gui)
{
    for (const auto& message : attackMessagesWhole)
    {
        gui.add_display_message(message);
    }
    attackMessagesWhole.clear();
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
    // Scrollable debug log viewer — not yet ported from curses to Raylib.
    // The log() function already streams to std::clog / std::cout in debug mode.
    // When implemented: read log file and render via ctx.renderer with scroll support.
    log("display_debug_messages: debug viewer not yet implemented for Raylib");
}
