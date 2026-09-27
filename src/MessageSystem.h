#pragma once

#include "Colors.h"

#include <string>
#include <vector>

#include "LogMessage.h"

class Gui;

// Whether a message stands on its own or is the opening of one built in parts.
// A message is often assembled from several coloured runs - "You hit " in white,
// the monster's name in its own colour - and the log holds them as one entry, so
// the caller has to say which it is writing.
enum class MessageCompletion
{
	CONTINUED, // more parts follow, added with append_message_part
	FINISHED, // the line is whole and closes the log entry
};

// - Handles all messaging and logging functionality
class MessageSystem
{
public:
	MessageSystem();
	~MessageSystem() = default;
	MessageSystem(const MessageSystem&) = delete;
	MessageSystem& operator=(const MessageSystem&) = delete;
	MessageSystem(MessageSystem&&) = delete;
	MessageSystem& operator=(MessageSystem&&) = delete;

	// Core message functionality
	void message(ColorPairId color, std::string_view text, MessageCompletion completion);
	void append_message_part(ColorPairId color, std::string_view text);
	void finalize_message();
	void transfer_messages_to_gui(Gui& gui);

	// Debug logging
	void log(std::string_view message) const;
	void display_debug_messages() const noexcept;

	// Getters for current message state
	const std::string& get_current_message() const noexcept { return messageToDisplay; }
	ColorPairId get_current_message_color() const noexcept { return messageColor; }

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
	ColorPairId messageColor{ ColorPairId::WHITE_BLACK };

	// Debug state
	bool debugMode{ true };

};
