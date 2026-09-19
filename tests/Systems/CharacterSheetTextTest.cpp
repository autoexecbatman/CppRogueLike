// file: CharacterSheetTextTest.cpp
// The character sheet's Constitution line against the book. Table 3 gives
// Constitution 17 +3 hit points a die for a warrior and +2 for every other class;
// the Player's Handbook (PDF page 32) adds it to each hit die rolled, and warriors
// and priests roll through 9th level, rogues and wizards through 10th.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CharacterSheetTextTest.*

#include <gtest/gtest.h>

#include <string>

#include "src/CharacterSheetUI.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int CONSTITUTION = 17;

} // namespace

class CharacterSheetTextTest : public ::testing::Test
{
protected:
	// The line the sheet draws for a character of this class and level at Constitution 17.
	std::string line_for(CreatureClass creatureClass, int level)
	{
		Creature character{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "character", 0 } };
		character.set_creature_class(creatureClass);
		character.set_creature_level(level);
		character.set_constitution(CONSTITUTION);
		return CharacterSheetText::constitution_line(character, mock.data_manager);
	}

	MockGameContext mock{};
};

// While the next level still rolls a die, the bonus is still a gain per level.
TEST_F(CharacterSheetTextTest, BeforeTheDiceStopTheBonusIsPerLevel)
{
	EXPECT_EQ(line_for(CreatureClass::FIGHTER, 8), "CON: 17  (+3 HP/level)");
	EXPECT_EQ(line_for(CreatureClass::WIZARD, 3), "CON: 17  (+2 HP/level)");
	EXPECT_EQ(line_for(CreatureClass::WIZARD, 9), "CON: 17  (+2 HP/level)");
}

// Once the next level rolls none, the line names the last level that took the bonus.
TEST_F(CharacterSheetTextTest, OnceTheDiceStopTheLineNamesTheLastLevelThatTookIt)
{
	EXPECT_EQ(line_for(CreatureClass::FIGHTER, 9), "CON: 17  (+3 HP/level through level 9)");
	EXPECT_EQ(line_for(CreatureClass::FIGHTER, 12), "CON: 17  (+3 HP/level through level 9)");
	EXPECT_EQ(line_for(CreatureClass::CLERIC, 12), "CON: 17  (+2 HP/level through level 9)");
	EXPECT_EQ(line_for(CreatureClass::ROGUE, 12), "CON: 17  (+2 HP/level through level 10)");
	EXPECT_EQ(line_for(CreatureClass::WIZARD, 10), "CON: 17  (+2 HP/level through level 10)");
}
