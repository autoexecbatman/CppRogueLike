// file: RangedWeaponTest.cpp
// Whether a creature can attack at a distance is a fact about what is in its
// missile slot, asked when it matters. It was an ActorState flag written at
// four sites and kept in step by two resynchronisers; this pins the derivation
// that replaced them.
//
// No behaviour changed: the flag was correct everywhere it could be observed,
// because can_equip already refuses anything but a ranged weapon in that slot.
// What the flag cost was two copies of one fact and the code to reconcile them.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/Actor/EquipmentSlot.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Core/Paths.h"
#include "src/Factories/ItemCreator.h"
#include "tests/mocks/MockGameContext.h"

class RangedWeaponTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ItemCreator::load(Paths::ITEMS);
		ctx = mock.to_game_context();

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);
		// The pack has to hold what comes back off, or unequipping asserts.
		player->set_strength(18);
		ctx.playerOwner = &player;
	}

	bool equip(std::string_view key, EquipmentSlot slot)
	{
		return player->equip_item(ItemCreator::create(key, Vector2D{ 0, 0 }, mock.content_registry), slot, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{};
};

// Bare-handed, there is nothing to shoot with.
TEST_F(RangedWeaponTest, WithAnEmptyMissileSlotThereIsNoRangedWeapon)
{
	EXPECT_FALSE(player->has_ranged_weapon());
}

// A bow in the missile slot is what makes a creature able to shoot.
TEST_F(RangedWeaponTest, ABowInTheMissileSlotIsARangedWeapon)
{
	ASSERT_TRUE(equip("long_bow", EquipmentSlot::MISSILE_WEAPON));

	EXPECT_TRUE(player->has_ranged_weapon());
}

// Taking it off ends it, with nothing to keep in step.
TEST_F(RangedWeaponTest, EmptyingTheSlotEndsIt)
{
	ASSERT_TRUE(equip("long_bow", EquipmentSlot::MISSILE_WEAPON));
	ASSERT_TRUE(player->unequip_item(EquipmentSlot::MISSILE_WEAPON, ctx));

	EXPECT_FALSE(player->has_ranged_weapon());
}

// A sword in hand is not a missile weapon, whatever else is worn.
TEST_F(RangedWeaponTest, AMeleeWeaponIsNotARangedWeapon)
{
	ASSERT_TRUE(equip("long_sword", EquipmentSlot::RIGHT_HAND));

	EXPECT_FALSE(player->has_ranged_weapon());
}

// The answer follows the slot rather than any other equipment moving: taking
// off armour while holding a bow leaves the bow exactly where it was. This is
// the case the old flag's resynchronisers existed to protect.
TEST_F(RangedWeaponTest, UnequippingSomethingElseLeavesItAlone)
{
	ASSERT_TRUE(equip("long_bow", EquipmentSlot::MISSILE_WEAPON));
	ASSERT_TRUE(equip("leather_armor", EquipmentSlot::BODY));
	ASSERT_TRUE(player->unequip_item(EquipmentSlot::BODY, ctx));

	EXPECT_TRUE(player->has_ranged_weapon());
}

// A monster with a bow answers the same way; nothing about this is player-only.
TEST_F(RangedWeaponTest, AMonsterAnswersFromItsOwnSlot)
{
	Creature archer{ Vector2D{ 1, 1 }, ActorData{ TileRef{}, "archer", 0 } };
	archer.healthPool = std::make_unique<HealthPool>(10);
	archer.set_body_plan({ EquipmentSlot::MISSILE_WEAPON });
	EXPECT_FALSE(archer.has_ranged_weapon());

	archer.wear(ItemCreator::create("short_bow", Vector2D{ 1, 1 }, mock.content_registry), EquipmentSlot::MISSILE_WEAPON);

	EXPECT_TRUE(archer.has_ranged_weapon());
}

// Creature::wear checks that the body has the slot, not what goes into it, and a
// monster's equipment comes from data. A dagger named for the missile slot is
// still not something to shoot with.
TEST_F(RangedWeaponTest, AMeleeWeaponWornInTheMissileSlotIsStillNotRanged)
{
	Creature confused{ Vector2D{ 1, 1 }, ActorData{ TileRef{}, "kobold", 0 } };
	confused.healthPool = std::make_unique<HealthPool>(10);
	confused.set_body_plan({ EquipmentSlot::MISSILE_WEAPON });

	confused.wear(ItemCreator::create("dagger", Vector2D{ 1, 1 }, mock.content_registry), EquipmentSlot::MISSILE_WEAPON);

	EXPECT_FALSE(confused.has_ranged_weapon())
		<< "the slot was read but what sits in it was not";
}
