// file: MimicStatBlockTest.cpp
// The mimic is the Monstrous Manual's killer mimic, the entry's second column (PDF page
// 1700 of the local archive): Hit Dice 9-10, THAC0 11, damage 3-12 (smash), and XP 2,000
// at 9 HD or 3,000 at 10. Each expectation below is that block's, read from the page.
// The owner set its first depth at 6, beside the monsters with the same THAC0 and XP.
//
// The scripted rolls are taken in the order the constructor draws them: one 3d6 for
// each of the six scores (Constitution read from Table 8, large), the number of hit
// dice, then each hit die.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MimicStatBlockTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>
#include <vector>

#include "src/GameContext.h"
#include "src/MonsterFactory.h"
#include "src/Monsters.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A large creature rolling 15 on Table 8 has Constitution 18, worth 4 a die.
constexpr int CONSTITUTION_BONUS_PER_DIE = 4;
constexpr int HIT_DIE_ROLLED = 1;

bool lists(const std::vector<MonsterPercentage>& distribution, std::string_view name)
{
	auto is_named = [name](const MonsterPercentage& entry)
	{
		return entry.name == name;
	};
	return std::ranges::any_of(distribution, is_named);
}
} // namespace

class MimicStatBlockTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// The six scores, then how many hit dice, then a 1 on each of them.
	void script_a_mimic_of(int hitDice)
	{
		for (const int roll : { 3, 3, 3, 3, 3, 3, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1 })
		{
			mock.dice.set_next_roll(roll);
		}
		mock.dice.set_next_roll(hitDice);
		for (int die = 0; die < hitDice; ++die)
		{
			mock.dice.set_next_roll(HIT_DIE_ROLLED);
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
};

TEST_F(MimicStatBlockTest, NineHitDiceMakeANinthLevelMimicWorthTwoThousand)
{
	script_a_mimic_of(9);
	const Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	EXPECT_EQ(mimic.get_creature_level(), 9);
	EXPECT_EQ(mimic.get_max_hp(), 9 * (HIT_DIE_ROLLED + CONSTITUTION_BONUS_PER_DIE));
	EXPECT_EQ(mimic.get_xp(), 2000);
}

TEST_F(MimicStatBlockTest, TenHitDiceMakeATenthLevelMimicWorthThreeThousand)
{
	script_a_mimic_of(10);
	const Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	EXPECT_EQ(mimic.get_creature_level(), 10);
	EXPECT_EQ(mimic.get_max_hp(), 10 * (HIT_DIE_ROLLED + CONSTITUTION_BONUS_PER_DIE));
	EXPECT_EQ(mimic.get_xp(), 3000);
}

TEST_F(MimicStatBlockTest, ItHitsAndSmashesAsTheBookGives)
{
	script_a_mimic_of(9);
	const Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	EXPECT_EQ(mimic.get_thaco(), 11);
	EXPECT_EQ(mimic.get_armor_class(), 7);
	// 3-12 is 3d4.
	EXPECT_EQ(mimic.attacker->get_damage_info().displayRoll, "3d4");
}

TEST_F(MimicStatBlockTest, ItFirstAppearsAtDepthSix)
{
	EXPECT_FALSE(lists(MonsterFactory::get_current_distribution(5, mock.monsterRegistry), "Mimic"));
	EXPECT_TRUE(lists(MonsterFactory::get_current_distribution(6, mock.monsterRegistry), "Mimic"));
}

// end of file: MimicStatBlockTest.cpp
