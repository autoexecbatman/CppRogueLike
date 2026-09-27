#pragma once

#include <string>

#include "Colors.h"

struct LogMessage
{
	ColorPairId logMessageColor{ ColorPairId::WHITE_BLACK };
	std::string logMessageText{};
};
