// file: ClassDisplayNameTest.cpp
// A class's name is derived from its enum by class_display_name, and nothing stores it.
// The blueprint, the character and the save all hold CreatureClass alone, so there is
// no name to read back and no parser to disagree with the table.
//
// What is checked here is the table itself, over every class a player can be: each has
// a name, and no two share one. What it does not check is the spelling, which is a
// presentation choice a test would have to be edited to change.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ClassDisplayNameTest.*

#include <gtest/gtest.h>

#include <set>
#include <string>

#include "src/CreatureClass.h"
#include "src/Player.h"

// Two classes sharing a name would show the player two choices they cannot tell apart.
TEST(ClassDisplayNameTest, NoTwoClassesShareAName)
{
	std::set<std::string> seen;
	for (const CreatureClass creatureClass : ALL_PLAYER_CLASS)
	{
		const std::string name{ class_display_name(creatureClass) };
		EXPECT_TRUE(seen.insert(name).second) << "'" << name << "' names more than one class";
	}
}

// A class is never nameless: the menu would offer an empty line.
TEST(ClassDisplayNameTest, NoClassIsNameless)
{
	for (const CreatureClass creatureClass : ALL_PLAYER_CLASS)
	{
		EXPECT_FALSE(class_display_name(creatureClass).empty())
			<< encode_creature_class(creatureClass) << " has no name to offer";
	}
}

// The choices are what the menu offers, so a monster's class must not be on the list.
TEST(ClassDisplayNameTest, AMonsterIsNotOneOfTheChoices)
{
	for (const CreatureClass creatureClass : ALL_PLAYER_CLASS)
	{
		EXPECT_NE(creatureClass, CreatureClass::MONSTER) << "the class menu offers the monster's class";
	}
}

// end of file: ClassDisplayNameTest.cpp
