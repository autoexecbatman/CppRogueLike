// file: EditorField.cpp
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "EditorField.h"

void refuse_misrouted_field(std::string_view label, std::string_view path)
{
	std::cerr << path << " has no case for the field '" << label << "'\n";
	std::abort();
}

// end of file: EditorField.cpp
