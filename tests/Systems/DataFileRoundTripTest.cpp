// file: DataFileRoundTripTest.cpp
// A shipped data file comes back byte for byte from a save that changed
// nothing: it is loaded through its own loader, saved through its own writer
// to a scratch file, and compared with the shipped bytes. Line endings are
// made LF on both sides first, because the checkout writes CRLF and the web
// build writes LF. Items, monsters, spells and tile_config still come back
// with their keys reordered, so their cases wait in issues/0400.
//
// On a mismatch the failure names the first line that differs, so a writer
// whose indent, key order or number format is not the file's says where.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DataFileRoundTripTest.*

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/ContentRegistry.h"
#include "src/ContentRegistryIO.h"
#include "src/DataFileText.h"
#include "src/Paths.h"
#include "src/PrefabLibrary.h"
#include "tests/mocks/TempTestPath.h"

namespace CppRL
{

	namespace
	{

		// A file's text with every CRLF made LF.
		std::string lf_text(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			const std::string raw{
				std::istreambuf_iterator<char>(file),
				std::istreambuf_iterator<char>(),
			};
			std::string text{};
			text.reserve(raw.size());
			for (size_t index = 0; index < raw.size(); ++index)
			{
				// A CR is dropped only where an LF follows it.
				if (raw[index] == '\r' && index + 1 < raw.size() &&
					raw[index + 1] == '\n')
				{
					continue;
				}
				text.push_back(raw[index]);
			}
			return text;
		}

		// Where two texts first part, by line, or empty when they are equal.
		//
		// Example:
		//   first_difference("a\nb\n", "a\nc\n");
		//   // -> "line 2: shipped \"b\", saved \"c\""
		//   first_difference("a\n", "a");
		//   // -> "line 2: shipped ends with a line break, saved does not"
		std::string first_difference(
			std::string_view shipped,
			std::string_view saved)
		{
			if (shipped == saved)
			{
				return {};
			}
			size_t line = 1;
			// Every line before the one that differs is equal, so both texts
			// reach it at the same offset.
			size_t start = 0;
			while (true)
			{
				const size_t shippedEnd = shipped.find('\n', start);
				const size_t savedEnd = saved.find('\n', start);
				// A count past the end is cut at the end, so npos reads the
				// last line whole.
				const std::string_view shippedLine =
					shipped.substr(start, shippedEnd - start);
				const std::string_view savedLine =
					saved.substr(start, savedEnd - start);
				if (shippedLine != savedLine)
				{
					return std::format(
						"line {}: shipped \"{}\", saved \"{}\"",
						line,
						shippedLine.substr(0, 90),
						savedLine.substr(0, 90));
				}

				// Equal lines, and one text stops here: the other runs on past
				// a line break this one lacks.
				const bool isShippedLast = shippedEnd == std::string_view::npos;
				const bool isSavedLast = savedEnd == std::string_view::npos;
				if (isShippedLast || isSavedLast)
				{
					const std::string_view shippedEnding =
						isShippedLast ? "does not end" : "ends";
					const std::string_view savedEnding =
						isSavedLast ? "does not" : "does";
					return std::format(
						"line {}: shipped {} with a line break, saved {}",
						line + 1,
						shippedEnding,
						savedEnding);
				}
				start = shippedEnd + 1;
				++line;
			}
		}

	} // namespace

	// The document at the indent asked for, and one line break after it.
	TEST(DataFileRoundTripTest, ADataFileEndsWithOneLineBreak)
	{
		EXPECT_EQ(
			data_file_text(nlohmann::json{ { "rat", 1 } }, 2),
			"{\n  \"rat\": 1\n}\n");
	}

	TEST(DataFileRoundTripTest, TilesComeBackAsShipped)
	{
		const std::filesystem::path scratch = temp_test_path("tiles.json");
		ContentRegistry tiles{};
		ContentRegistryIO::load(tiles, Paths::CONTENT_TILES);

		ContentRegistryIO::save(tiles, scratch.string());
		const std::string saved = lf_text(scratch);
		std::filesystem::remove(scratch);

		const std::string difference = first_difference(
			lf_text(Paths::resolve(Paths::CONTENT_TILES)),
			saved);
		EXPECT_TRUE(difference.empty()) << difference;
	}

	TEST(DataFileRoundTripTest, PrefabsComeBackAsShipped)
	{
		const std::filesystem::path scratch = temp_test_path("prefabs.json");
		PrefabLibrary prefabs{};
		prefabs.load(Paths::PREFABS);

		prefabs.save(scratch.string());
		const std::string saved = lf_text(scratch);
		std::filesystem::remove(scratch);

		const std::string difference =
			first_difference(lf_text(Paths::resolve(Paths::PREFABS)), saved);
		EXPECT_TRUE(difference.empty()) << difference;
	}

} // namespace CppRL
