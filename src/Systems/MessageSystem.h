// MessageSystem.h - Handles all messaging and logging functionality

#ifndef MESSAGESYSTEM_H
#define MESSAGESYSTEM_H

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include "../Gui/LogMessage.h"

class Gui;

class MessageSystem
{
public:
    MessageSystem();
    ~MessageSystem() = default;

    // Core message functionality
    void message(int color, std::string_view text, bool isComplete = false);
    void append_message_part(int color, std::string_view text);
    void finalize_message();
    void transfer_messages_to_gui(Gui& gui);

    // Debug logging
    void log(std::string_view message) const;
    void display_debug_messages() const noexcept;

    // Getters for current message state
    const std::string& get_current_message() const noexcept { return messageToDisplay; }
    int get_current_message_color() const noexcept { return messageColor; }

    // Debug mode control
    void enable_debug_mode() noexcept { debugMode = true; }
    void disable_debug_mode() noexcept { debugMode = false; }
    bool is_debug_mode() const noexcept { return debugMode; }

    // Get size of stored messages
    size_t get_stored_message_count() const noexcept { return attackMessagesWhole.size(); }
    // Get attack messages whole at index
    const std::vector<LogMessage>& get_attack_message_at(size_t index) const
    {
            return attackMessagesWhole.at(index);
    }

private:
    // Message storage
    std::vector<LogMessage> attackMessageParts;
    std::vector<std::vector<LogMessage>> attackMessagesWhole;
    std::string messageToDisplay{ "Init Message" };
    int messageColor{ 0 };
    
    // Debug state
    bool debugMode{ true };
    
    // Helper methods for debug display
    void render_debug_background() const;
    void create_debug_pad(int total_lines) const;
};

#endif // MESSAGESYSTEM_H
