// file: LevelUpHitDieTest.cpp
// A level's hit points are rolled on the hit die the creature carries, whatever its size.
// Real dice, over many level-ups: a scripted value outside the die the code asks for
// aborts on RandomDice's range check rather than failing an assertion.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=LevelUpHitDieTest.*

#include <gtest/gtest.h>

#include <memory>
#include <set>

#include "src/ArmorClass.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/LevelUpSystem.h"
#include "src/Player.h"
#include "src/Vector2D.h"
#include "tests/mocks/TestDice.h"

namespace
{
// Level-ups measured per die. The chance that 200 real d12 rolls all land at 8 or under
// is (8/12)^200, about 6e-36; that 200 d2 rolls all land on one face, about 1e-60.
constexpr int LEVEL_UPS = 200;
// Below 20, Table 3 floors a rolled hit die at 1 and nowhere higher, so any roll of 1 or
// more is the gain itself; a roll of 0 would read as 1, which is why no test asks for one.
constexpr int CONSTITUTION_WITH_NO_RAISED_FLOOR = 10;
} // namespace

class LevelUpHitDieTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		fighter->experienceReward = std::make_unique<ExperienceReward>(0);
		fighter->armorClass = std::make_unique<ArmorClass>(10);
		fighter->healthPool = std::make_unique<HealthPool>(10);
		fighter->set_creature_class(CreatureClass::FIGHTER);
		fighter->set_constitution(CONSTITUTION_WITH_NO_RAISED_FLOOR);
		ctx = game.context();
		ctx.playerOwner = &fighter;
		// Test mode with nothing queued serves the real generator.
		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// The die each of LEVEL_UPS level-ups to 2nd added to the fighter's base hit points.
	std::multiset<int> rolls_on_a_hit_die_of(int faces)
	{
		fighter->set_hit_die(faces);
		std::multiset<int> rolled;
		for (int levelUp = 0; levelUp < LEVEL_UPS; ++levelUp)
		{
			const int before = fighter->get_hp_base();
			LevelUpSystem::apply_level_up_benefits(*fighter, 2, ctx);
			rolled.insert(fighter->get_hp_base() - before);
		}
		return rolled;
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx{};
	std::unique_ptr<Player> fighter{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

// A d12 rolls from 1 to 12, so some of 200 rolls land above 8.
TEST_F(LevelUpHitDieTest, ATwelveSidedHitDieRollsTwelveSided)
{
	const std::multiset<int> rolled = rolls_on_a_hit_die_of(12);

	EXPECT_LE(*rolled.rbegin(), 12);
	EXPECT_GT(*rolled.rbegin(), 8) << "200 rolls on a d12 never passed 8, which is a d8";
}

// A d2 rolls 1 or 2, and 200 rolls show both.
TEST_F(LevelUpHitDieTest, ATwoSidedHitDieRollsTwoSided)
{
	const std::multiset<int> rolled = rolls_on_a_hit_die_of(2);

	EXPECT_EQ(*rolled.begin(), 1);
	EXPECT_EQ(*rolled.rbegin(), 2) << "a roll outside 1 to 2 came from another die";
}
