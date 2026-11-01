// file: DataFileText.h
// The text a data file the repository ships is written as, so every writer of
// one ends it the same way: the document at the file's own indent, then one
// line break. The indent is the writer's, because each shipped file keeps the
// one it was checked in with.
//
// Example:
//   data_file_text(nlohmann::json{ { "rat", 1 } }, 2);
//   // -> "{\n  \"rat\": 1\n}\n"
//   file << data_file_text(root, 4); // in place of file << root.dump(4)
#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace CppRL
{

	// The document dumped at that indent, followed by one line break.
	//
	// Example:
	//   data_file_text(nlohmann::json{ { "rat", 1 } }, 2);
	//   // -> "{\n  \"rat\": 1\n}\n"
	[[nodiscard]] std::string data_file_text(
		const nlohmann::json& document,
		int indent);

} // namespace CppRL
