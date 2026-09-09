// file: AttitudeTest.cpp
// Verifies the attitude scale's threshold predicate and the state accessors that
// replaced Ai::is_hostile(). Attitude is creature state, so it must be settable
// at runtime rather than fixed by which Ai subclass a creature carries.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/Utils/Vector2D.h"

class AttitudeTest : public ::testing::Test
{
protected:
	Creature creature{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", 0 } };
};

// Creatures are hostile unless something says otherwise.
TEST_F(AttitudeTest, DefaultsToHostile)
{
	EXPECT_EQ(creature.get_attitude(), Attitude::HOSTILE);
}

// Attitude is runtime state, which is the whole reason it left the Ai class.
TEST_F(AttitudeTest, IsMutableAtRuntime)
{
	creature.set_attitude(Attitude::PEACEFUL);

	EXPECT_EQ(creature.get_attitude(), Attitude::PEACEFUL);
}

// Only a hostile creature may be attacked without asking.
TEST_F(AttitudeTest, ConfirmationRequiredFromPeacefulUpward)
{
	EXPECT_FALSE(needs_attack_confirmation(Attitude::HOSTILE));
	EXPECT_TRUE(needs_attack_confirmation(Attitude::PEACEFUL));
	EXPECT_TRUE(needs_attack_confirmation(Attitude::FRIENDLY));
	EXPECT_TRUE(needs_attack_confirmation(Attitude::TAME));
}

// The scale is ordered, so threshold comparisons mean what they read as.
TEST_F(AttitudeTest, ScaleIsOrderedFromHostileToTame)
{
	EXPECT_LT(Attitude::HOSTILE, Attitude::PEACEFUL);
	EXPECT_LT(Attitude::PEACEFUL, Attitude::FRIENDLY);
	EXPECT_LT(Attitude::FRIENDLY, Attitude::TAME);
}

// Creatures may be pushed past by default; unique NPCs opt out.
TEST_F(AttitudeTest, DisplaceableDefaultsTrueAndIsSettable)
{
	EXPECT_TRUE(creature.is_displaceable());

	creature.set_displaceable(false);

	EXPECT_FALSE(creature.is_displaceable());
}
