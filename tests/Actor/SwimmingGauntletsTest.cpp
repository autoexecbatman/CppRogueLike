// file: SwimmingGauntletsTest.cpp
// What the gauntlets of swimming and climbing do, against the book that describes them.
//
// Dungeon Master Guide, PDF pages 964-965: "The wearer can swim as fast as a triton
// (movement of 15) underwater, and as fast as a merman (movement 18) on the surface.
// These gauntlets do not empower the wearer to breathe in water... He can climb
// vertical or nearly vertical surfaces, upward or downward, with a 95% chance of
// success." Nothing in the entry touches Strength.
//
// The defect this pins: items.json gave them strBonus 2, which reached the attack roll,
// the damage, the carrying capacity and the open-doors chance - a real mechanical
// effect the book never granted them.
//
// What they grant instead is the game's whole form of swimming: water is a tile a
// creature crosses if it can swim, so the gauntlets carry their wearer across it. The
// climbing half has nothing to attach to, since the game has no climbing.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SwimmingGauntletsTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ItemCreator.h"
#include "src/MagicalItemEffects.h"
#include "src/Pickable.h"
#include "tests/mocks/MockGameContext.h"

class SwimmingGauntletsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		swimmer.set_body_plan(mock.body_plans.get("humanoid"));
		swimmer.set_strength(12);
	}

	std::unique_ptr<Item> the_gauntlets()
	{
		return ItemCreator::create("gauntlets_of_swimming_and_climbing", Vector2D{ 0, 0 }, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Creature swimmer{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "swimmer", 0 } };
};

// The book's entry grants no Strength, so neither does the item.
TEST_F(SwimmingGauntletsTest, TheyGrantNoStrength)
{
	const auto gauntlets = the_gauntlets();
	ASSERT_TRUE(gauntlets->behavior.has_value());
	const Gauntlets* worn = std::get_if<Gauntlets>(&*gauntlets->behavior);
	ASSERT_NE(worn, nullptr);

	EXPECT_EQ(worn->strBonus, 0) << "the book's entry says nothing about Strength";
	EXPECT_EQ(worn->dexBonus, 0);
	EXPECT_EQ(worn->conBonus, 0);
}

// Putting them on does not move the wearer's Strength either, which is the number the
// old bonus reached: the attack roll, the damage and the carrying capacity all read it.
TEST_F(SwimmingGauntletsTest, WearingThemLeavesTheWearersStrengthAlone)
{
	const int before = swimmer.get_strength();

	swimmer.wear(the_gauntlets(), EquipmentSlot::GAUNTLETS);

	EXPECT_EQ(swimmer.get_strength(), before);
}

// What they do grant: the effect the game reads when a creature meets water.
TEST_F(SwimmingGauntletsTest, TheyCarryTheSwimmingEffect)
{
	const auto gauntlets = the_gauntlets();
	const Gauntlets* worn = std::get_if<Gauntlets>(&*gauntlets->behavior);
	ASSERT_NE(worn, nullptr);

	EXPECT_EQ(worn->effect, MagicalEffect::SWIMMING);
}

// A creature that cannot swim of its own can swim while wearing them, and stops when
// they come off. Nothing is copied onto the creature, so there is nothing to undo.
TEST_F(SwimmingGauntletsTest, TheWearerCrossesWaterAndStopsWhenTheyComeOff)
{
	ASSERT_FALSE(swimmer.has_state(ActorState::CAN_SWIM));
	EXPECT_FALSE(swimmer.has_bypass(ActorState::CAN_SWIM));

	swimmer.wear(the_gauntlets(), EquipmentSlot::GAUNTLETS);

	EXPECT_TRUE(swimmer.has_bypass(ActorState::CAN_SWIM));
	EXPECT_FALSE(swimmer.has_state(ActorState::CAN_SWIM)) << "a worn source is asked, never copied";

	// Taking them off is removing the entry, which is what Player::unequip_item does.
	// Creature::unequip only clears a flag and has no caller in src.
	swimmer.equippedItems.clear();

	EXPECT_FALSE(swimmer.has_bypass(ActorState::CAN_SWIM));
}

// A creature that swims of its own needs no gauntlets, and the gauntlets grant nothing
// else: they are not a skeleton key for every tile a state guards.
TEST_F(SwimmingGauntletsTest, TheyGrantSwimmingAndNothingElse)
{
	swimmer.wear(the_gauntlets(), EquipmentSlot::GAUNTLETS);

	EXPECT_TRUE(swimmer.has_bypass(ActorState::CAN_SWIM));
	EXPECT_FALSE(swimmer.has_bypass(ActorState::CAN_WALK_WEBS));

	Creature spider{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "spider", 0 } };
	spider.add_state(ActorState::CAN_SWIM);

	EXPECT_TRUE(spider.has_bypass(ActorState::CAN_SWIM)) << "a creature's own state still answers";
}

// Another effect-bearing item worn in the same slot grants no swimming: the effect is
// compared, not merely found. Gauntlets of ogre power set Strength and nothing else.
TEST_F(SwimmingGauntletsTest, AnItemWithADifferentEffectGrantsNoSwimming)
{
	swimmer.wear(ItemCreator::create("gauntlets_of_ogre_power", Vector2D{ 0, 0 }, ctx), EquipmentSlot::GAUNTLETS);

	EXPECT_FALSE(swimmer.wears_item_with(MagicalEffect::SWIMMING));
	EXPECT_FALSE(swimmer.has_bypass(ActorState::CAN_SWIM));
}
