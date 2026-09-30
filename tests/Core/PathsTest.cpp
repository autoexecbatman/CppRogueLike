// file: PathsTest.cpp
// Every runtime path the game uses has to be a name the filesystem will accept.
//
// Paths.h calls itself the single source of truth for runtime paths, and a
// constant there is used by exactly one line of code, so nothing else says what
// the name should look like. A name the operating system refuses does not fail at
// the constant - it fails much later, in whichever stream tries to open it, and
// on Windows it fails for every save the game ever attempts.
//
// The constants are read out of the header rather than listed here, so a path
// added to Paths.h is covered by being written. A list in a test is a second
// place to maintain and it drifts silently.
//
// What this does not check: that the file exists, or that the path points
// anywhere useful. Only that the name is one the platform can create.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PathsTest.*

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "src/Paths.h"

namespace
{
// One constant as the header declares it.
struct DeclaredPath
{
	std::string name{};
	std::string value{};
};

// Reads Paths.h and returns every string_view constant it declares.
std::vector<DeclaredPath> declared_paths()
{
	std::vector<DeclaredPath> found;
	std::ifstream header(Paths::resolve("src/Paths.h"));
	// A custom delimiter, because the pattern itself contains the )" that would
	// otherwise close the raw string.
	const std::regex declaration{ R"REGEX(inline\s+constexpr\s+std::string_view\s+(\w+)\s*=\s*"([^"]*)")REGEX" };

	std::string line;
	while (std::getline(header, line))
	{
		std::smatch matched;
		if (std::regex_search(line, matched, declaration))
		{
			found.push_back({ matched[1].str(), matched[2].str() });
		}
	}
	return found;
}

// Creates the name under a directory of its own and reports whether the
// filesystem took it. Asking the platform beats listing the characters it
// reserves, because the list differs per platform and this has to hold on the
// one the game ships from.
bool is_creatable(const std::string& relative, const std::filesystem::path& sandbox)
{
	const std::filesystem::path target = sandbox / relative;
	std::error_code problem;
	std::filesystem::create_directories(target.parent_path(), problem);
	if (problem)
	{
		return false;
	}

	std::ofstream probe(target);
	const bool opened = probe.is_open();
	probe.close();
	std::filesystem::remove(target, problem);
	return opened;
}
} // namespace

class PathsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		sandbox = std::filesystem::temp_directory_path() / "cpprogue_paths_test";
		std::filesystem::remove_all(sandbox);
		std::filesystem::create_directories(sandbox);
	}

	void TearDown() override
	{
		std::error_code ignored;
		std::filesystem::remove_all(sandbox, ignored);
	}

	std::filesystem::path sandbox{};
};

// The header is the source of the list, so an empty read means this case is
// measuring nothing rather than passing.
TEST_F(PathsTest, TheHeaderDeclaresPathsToCheck)
{
	const std::vector<DeclaredPath> paths = declared_paths();

	ASSERT_FALSE(paths.empty()) << "no constants were read out of src/Paths.h, so every case below is vacuous";
	EXPECT_GT(paths.size(), 5u) << "far fewer constants than Paths.h declares; the pattern stopped matching";
}

// A name the platform refuses is a file the game can never write.
TEST_F(PathsTest, EveryDeclaredPathIsANameTheFilesystemAccepts)
{
	for (const DeclaredPath& path : declared_paths())
	{
		EXPECT_TRUE(is_creatable(path.value, sandbox))
			<< "Paths::" << path.name << " is \"" << path.value
			<< "\", which this filesystem will not create";
	}
}

// The save is the one path the game writes rather than reads, and it writes it
// twice: save_game opens a temporary beside it first. A name with no extension
// makes replace_extension append instead of replace, so the temporary inherits
// whatever is wrong with the name.
TEST_F(PathsTest, TheSaveFileAndItsTemporaryAreBothCreatable)
{
	const std::filesystem::path save{ std::string(Paths::SAVE_FILE) };

	EXPECT_TRUE(is_creatable(std::string(Paths::SAVE_FILE), sandbox)) << "the game cannot write its save file";

	std::filesystem::path temporary = save;
	temporary.replace_extension(".tmp");
	EXPECT_TRUE(is_creatable(temporary.generic_string(), sandbox)) << "the game cannot write the temporary it saves through";
}

// end of file: PathsTest.cpp
