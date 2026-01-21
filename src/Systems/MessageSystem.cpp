#include <iostream>
#include <format>
#include <fstream>

#include <curses.h>

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
    // Clear screen and render game background before showing debug screen
    clear();
    refresh();
}

void MessageSystem::create_debug_pad(int total_lines) const
{
    // Create a pad large enough to hold all the text
    WINDOW* log_pad = newpad(total_lines + 1, COLS - 2);
    int y = 0;

    // Open the file again to actually display the text
    std::ifstream logFile("clog.txt");
    if (logFile.is_open())
    {
        std::string line;
        while (getline(logFile, line))
        {
            mvwprintw(log_pad, y++, 1, "%s", line.c_str());
        }
        logFile.close();
    }

    // Initial display position
    int pad_pos = 0;
    prefresh(log_pad, pad_pos, 0, 1, 1, LINES - 2, COLS - 2);

    // Scroll interaction
    int ch;
    do
    {
        ch = getch();
        switch (ch)
        {
        case KEY_DOWN:
            if (pad_pos + LINES - 2 < total_lines)
            {
                pad_pos++;
            }
            break;
        case KEY_UP:
            if (pad_pos > 0)
            {
                pad_pos--;
            }
            break;
        case KEY_NPAGE:  // Handle Page Down
            if (pad_pos + LINES - 2 < total_lines)
            {
                pad_pos += (LINES - 2);  // Move down a page
                if (pad_pos + LINES - 2 > total_lines)
                {  // Don't go past the end
                    pad_pos = total_lines - LINES + 2;
                }
            }
            break;
        case KEY_PPAGE:  // Handle Page Up
            if (pad_pos > 0)
            {
                pad_pos -= (LINES - 2);  // Move up a page
                if (pad_pos < 0)
                {
                    pad_pos = 0;  // Don't go past the beginning
                }
            }
            break;
        case KEY_HOME:  // Jump to the top of the log
            pad_pos = 0;
            break;
        case KEY_END:  // Jump to the bottom of the log
            if (total_lines > LINES - 2)
            {
                pad_pos = total_lines - LINES + 2;
            }
            break;
        }
        prefresh(log_pad, pad_pos, 0, 1, 1, LINES - 2, COLS - 2);
    } while (ch != 'q' && ch != 27);  // Exit on 'q' or Escape

    delwin(log_pad);  // Delete the pad after use
}
