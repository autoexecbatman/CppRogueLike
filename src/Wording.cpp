// file: Wording.cpp
#include <cassert>
#include <format>
#include <string>
#include <string_view>

#include "Wording.h"

namespace Wording
{

std::string counted(int count, std::string_view singular, std::string_view plural)
{
	assert(count >= 0 && "Wording::counted: a message counted fewer than none");
	return std::format("{} {}", count, count == 1 ? singular : plural);
}

} // namespace Wording
