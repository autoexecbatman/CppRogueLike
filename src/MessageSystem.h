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
private:
	// Everything a run owns - the log and the line on screen - replaced whole by begin_run,
	// so a field added here starts each run fresh without being listed. Settings stay outside.
	struct RunState
	{
		std::vector<LogMessage> attackMessageParts{};
		std::vector<std::vector<LogMessage>> attackMessagesWhole{};
		std::string messageToDisplay{ "Init Message" };
		ColorPairId messageColor{ ColorPairId::WHITE_BLACK };
	};

	RunState run{};

	// Debug state
	bool debugMode{ true };

public:
	MessageSystem();
	~MessageSystem() = default;
	MessageSystem(const MessageSystem&) = delete;
	MessageSystem& operator=(const MessageSystem&) = delete;
	MessageSystem(MessageSystem&&) = delete;
	MessageSystem& operator=(MessageSystem&&) = delete;

	// Clears the log and the line on screen for a new run. Settings such as debug mode stay.
	//
	// Example, after a run that left lines in the log:
	//   messages.begin_run();
	//   messages.get_stored_message_count();   // -> 0
	void begin_run();

	// Core message functionality
	void message(
		ColorPairId color,
		std::string_view text,
		MessageCompletion completion);
	void append_message_part(ColorPairId color, std::string_view text);
	void finalize_message();
	void transfer_messages_to_gui(Gui& gui);

	// Debug logging
	void log(std::string_view message) const;
	void display_debug_messages() const noexcept;

	// Getters for current message state
	const std::string& get_current_message() const noexcept
	{
		return run.messageToDisplay;
	}

	ColorPairId get_current_message_color() const noexcept
	{
		return run.messageColor;
	}

	// Debug mode control
	void enable_debug_mode() noexcept { debugMode = true; }

	void disable_debug_mode() noexcept { debugMode = false; }

	bool is_debug_mode() const noexcept { return debugMode; }

	// Get size of stored messages
	size_t get_stored_message_count() const noexcept
	{
		return run.attackMessagesWhole.size();
	}

	// Get attack messages whole at index
	const std::vector<LogMessage>& get_attack_message_at(size_t index) const
	{
		return run.attackMessagesWhole.at(index);
	}
};
