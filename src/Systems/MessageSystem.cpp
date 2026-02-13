#include <iostream>
#include <format>
#include <fstream>

#include "MessageSystem.h"
#include "../Gui/Gui.h"
#include "../Colors/Colors.h"

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
    render_debug_background();

    int total_lines = 0;
    std::ifstream logFile("clog.txt");
    std::string line;

    // First, count the number of lines in the file
    while (getline(logFile, line))
    {
        total_lines++;
    }
    logFile.close();

    create_debug_pad(total_lines);
}

void MessageSystem::render_debug_background() const
{
    // TODO: stub - clear screen and render game background without curses
}

void MessageSystem::create_debug_pad(int total_lines) const
{
    // TODO: stub - debug log pad display requires curses replacement
    // Previously used newpad/prefresh/getch/delwin for scrollable log viewer
    // Key constants for reference:
    //   KEY_DOWN = 0x102, KEY_UP = 0x103
    //   KEY_HOME = 0x106, KEY_END = 0x166
}
