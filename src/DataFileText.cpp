// file: DataFileText.cpp
// The shipped data file layout. Usage is in DataFileText.h.

#include <nlohmann/json.hpp>
#include <string>

#include "DataFileText.h"

namespace CppRL
{

	std::string data_file_text(const nlohmann::json& document, int indent)
	{
		return document.dump(indent) + "\n";
	}

} // namespace CppRL
