// file: ProtectionFromEvilTest.cpp
// Verifies the AD&D 2e Protection From Evil to-hit penalty (PHB page 277): evil
// attackers suffer -2 against a warded creature, and non-evil attackers do not.
// The penalty is keyed to the attacker's alignment, which is the whole point of
// the spell, so both halves are asserted.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/Systems/BuffSystem.h"
#include "src/Systems/BuffType.h"
#include "src/Utils/Vector2D.h"

class ProtectionFromEvilTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		attacker.set_morality(Morality::EVIL);
	}

	// Wards the target for the given number of turns.
	void ward_target(int duration)
	{
		target.activeBuffs.push_back(
			Buff{ BuffType::PROTECTION_FROM_EVIL, PROTECTION_FROM_EVIL_PENALTY, duration, false });
	}

	BuffSystem buffs{};
	Creature attacker{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", 0 } };
	Creature target{ Vector2D{ 1, 0 }, ActorData{ TileRef{}, "cleric", 0 } };
};

// An unwarded target imposes nothing, however evil the attacker.
TEST_F(ProtectionFromEvilTest, UnwardedTargetImposesNoPenalty)
{
	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), 0);
}

// The book's number, applied to an evil attacker.
TEST_F(ProtectionFromEvilTest, EvilAttackerSuffersTwoPointPenalty)
{
	ward_target(6);

	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), -2);
}

// A non-evil attacker walks through the ward untouched.
TEST_F(ProtectionFromEvilTest, NonEvilAttackerIsUnaffected)
{
	ward_target(6);
	attacker.set_morality(Morality::NEUTRAL);

	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), 0);

	attacker.set_morality(Morality::GOOD);

	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), 0);
}

// Ethics are irrelevant: the spell reads the good/evil axis alone.
TEST_F(ProtectionFromEvilTest, PenaltyIgnoresTheEthicsAxis)
{
	ward_target(6);

	attacker.set_ethics(Ethics::LAWFUL);
	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), -2) << "lawful evil is still evil";

	attacker.set_ethics(Ethics::CHAOTIC);
	EXPECT_EQ(buffs.calculate_ward_penalty(attacker, target), -2);
}
