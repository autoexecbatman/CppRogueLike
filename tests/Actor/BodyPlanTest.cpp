#include "src/Actor/Creature.h"
#include "src/Actor/EquipmentSlot.h"
#include <gtest/gtest.h>

#include <vector>

// A body plan is the set of slots a creature has at all, which is a different
// question from what is currently in them. It is authored per creature, so a
// spider and a hobgoblin need not agree about what a body is.
class BodyPlanTest : public ::testing::Test
{
protected:
	static Creature make_creature()
	{
		ActorData data{};
		data.name = "test creature";
		return Creature(Vector2D(0, 0), data);
	}
};

// Nothing is assumed. A creature with no authored plan wears nothing.
TEST_F(BodyPlanTest, UnauthoredCreatureHasNoSlots)
{
	Creature creature = make_creature();

	EXPECT_FALSE(creature.has_slot(EquipmentSlot::RIGHT_HAND));
	EXPECT_FALSE(creature.has_slot(EquipmentSlot::BODY));
	EXPECT_FALSE(creature.has_slot(EquipmentSlot::HEAD));
}

// The slots named in the plan are the slots the creature has.
TEST_F(BodyPlanTest, AuthoredSlotsArePresent)
{
	Creature creature = make_creature();
	creature.set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::BODY });

	EXPECT_TRUE(creature.has_slot(EquipmentSlot::RIGHT_HAND));
	EXPECT_TRUE(creature.has_slot(EquipmentSlot::BODY));
}

// And only those. A plan grants what it lists, not a whole humanoid body.
TEST_F(BodyPlanTest, UnlistedSlotsAreAbsent)
{
	Creature creature = make_creature();
	creature.set_body_plan({ EquipmentSlot::RIGHT_HAND });

	EXPECT_FALSE(creature.has_slot(EquipmentSlot::LEFT_HAND));
	EXPECT_FALSE(creature.has_slot(EquipmentSlot::BOOTS));
	EXPECT_FALSE(creature.has_slot(EquipmentSlot::MISSILE_WEAPON));
}

// Setting a plan replaces the previous one rather than adding to it.
TEST_F(BodyPlanTest, SettingAPlanReplacesTheOldOne)
{
	Creature creature = make_creature();
	creature.set_body_plan({ EquipmentSlot::RIGHT_HAND });
	creature.set_body_plan({ EquipmentSlot::BOOTS });

	EXPECT_FALSE(creature.has_slot(EquipmentSlot::RIGHT_HAND));
	EXPECT_TRUE(creature.has_slot(EquipmentSlot::BOOTS));
}
