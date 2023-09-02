// file: LogMessage.h
#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H
//Create a struct to be able to define the color of each line in the log.
//So we need a structure to store the message's text and its color.
//Since this structure is only used by the Gui class, we put it in its protected declaration zone.

#include <string>

struct LogMessage
{
	int logMessageColor{ 0 };
	std::string logMessageText{};
};

#endif // !LOGMESSAGE_H
// end of file: LogMessage.h
