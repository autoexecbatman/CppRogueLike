// file: RangedShotTest.cpp
// What a fired missile shows. One function decides it for the player's shot and a
// monster's: blood only where the attack landed, the missile alone where it missed,
// and nothing where no shot was loosed - melee already draws its hit only on a hit.
//
// The animation cannot be stepped without a window, since update_and_render reads
// raylib's frame time and draws. So the blood rule is tested through impact_of, the
// query fire draws from, and fire is tested only for whether a missile was spawned at
// all. That blood appears on screen on arrival is shown by a screenshot, not here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RangedShotTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/AttackResult.h"
#include "src/Attacker.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/ItemCreator.h"
#include "src/MonsterAttacker.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class RangedShotTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		// A sling's 1d4 shot from THAC0 20, which needs a 10 against armour class 10.
		archer.experienceReward = std::make_unique<ExperienceReward>(0);
		archer.healthPool = std::make_unique<HealthPool>(10);
		archer.armorClass = std::make_unique<ArmorClass>(10);
		archer.attacker = std::make_unique<MonsterAttacker>(archer, DamageInfo{ "1d4", DamageType::PHYSICAL });
		archer.set_thaco(20);
		archer.set_strength(10);
		archer.set_body_plan({ EquipmentSlot::MISSILE_WEAPON });
		archer.wear(ItemCreator::create("sling", archer.position, ctx), EquipmentSlot::MISSILE_WEAPON);

		target.experienceReward = std::make_unique<ExperienceReward>(0);
		target.healthPool = std::make_unique<HealthPool>(100);
		target.armorClass = std::make_unique<ArmorClass>(10);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Creature archer{
		Vector2D{ 2, 5 },
		ActorData{ TileRef{}, "archer", ColorPairId::WHITE_BLACK },
	};
	Creature target{
		Vector2D{ 6, 5 },
		ActorData{ TileRef{}, "target", ColorPairId::WHITE_BLACK },
	};
};

// Each outcome earns its own picture, and only a landed attack draws blood.
TEST_F(RangedShotTest, OnlyALandedShotBleeds)
{
	EXPECT_EQ(RangedShot::impact_of(AttackResult::LANDED), ShotImpact::MISSILE_AND_BLOOD);
	EXPECT_EQ(RangedShot::impact_of(AttackResult::MISSED), ShotImpact::MISSILE) << "a miss drew blood";
	EXPECT_EQ(RangedShot::impact_of(AttackResult::PREVENTED), ShotImpact::NOTHING) << "a shot never loosed was drawn";
}

// A shot that is rolled flies, hit or miss.
TEST_F(RangedShotTest, ARolledShotLoosesAMissile)
{
	ASSERT_FALSE(ctx.animSystem->has_active_entries());

	// A natural 1 misses, and the damage die is rolled anyway.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(2);
	EXPECT_EQ(RangedShot::fire(archer, target, ctx), AttackResult::MISSED);

	EXPECT_TRUE(ctx.animSystem->has_active_entries()) << "a missed shot drew no missile";
}

// A target already dead is shot at in vain: nothing is rolled, so nothing flies.
TEST_F(RangedShotTest, AShotNeverLoosedDrawsNothing)
{
	target.set_hp(0);
	ASSERT_TRUE(target.is_dead());
	ASSERT_FALSE(ctx.animSystem->has_active_entries());

	EXPECT_EQ(RangedShot::fire(archer, target, ctx), AttackResult::PREVENTED);

	EXPECT_FALSE(ctx.animSystem->has_active_entries()) << "a shot that was never loosed was drawn";
}

// end of file: RangedShotTest.cpp
