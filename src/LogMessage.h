#pragma once

// file: LogMessage.h
//
// One run of text in one colour: the piece a message is built from rather than a
// message itself. A line in the log is a vector of these, so "You hit " in white and
// the monster's name in its own colour are two of them and one entry.
//
// Usage - MessageSystem collects the parts and hands the finished line to the Gui:
//
//   ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You hit ");
//   ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, monster.get_name());
//   ctx.messageSystem->finalize_message();   // the two parts become one log entry
//
// Read back, a part carries only what it is and what colour to draw it in:
//
//   for (const LogMessage& part : messageSystem.get_attack_message_at(index))
//   {
//       renderer.draw_text(where, part.text, part.color);
//   }

#include <string>

#include "Colors.h"

struct LogMessage
{
	ColorPairId color{ ColorPairId::WHITE_BLACK };
	std::string text{};
};

// end of file: LogMessage.h
