// file: RingOfProtectionTest.cpp A worn ring of protection betters armour
// class by its plus, and of two worn together only the stronger counts.
//
// What it is for. The DMG (PDF page 908): "A ring +1 betters AC by 1 (say,
// from 10 to 9)", and its table gives each ring's level of protection, +1 to
// +6, so a ring's plus is its armour class bonus. "Multiple rings of
// protection operating on the same person ... do not combine protection. Only
// one such ring - the strongest - functions, so a pair of protection rings +2
// provides only +2 protection."
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RingOfProtectionTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>
#include <utility>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

	namespace
	{
		// PHB Table 46: no armour.
		constexpr int BARE_ARMOR_CLASS = 10;
		// DMG page 908: a ring +1 betters AC by 1, a ring +2 by 2.
		constexpr int PLUS_ONE_ARMOR_CLASS = 9;
		constexpr int PLUS_TWO_ARMOR_CLASS = 8;
		// A creature built in a test has no Strength, and the pack is
		// weight-gated by it.
		constexpr int CARRIER_STRENGTH = 18;
		// Dexterity with no armour class adjustment (PHB Table 2).
		constexpr int NEUTRAL_DEXTERITY = 10;
	} // namespace

	class RingOfProtectionTest : public ::testing::Test
	{
	protected:
		MockGameContext mock{};
		GameContext ctx{};
		BuffSystem buffs{};

		void SetUp() override
		{
			ctx = mock.to_game_context();
			ctx.buffSystem = &buffs;
			mock.player = std::make_unique<Player>(Vector2D{ 0, 0 });
			Player& wearer = *mock.player;
			wearer.experienceReward = std::make_unique<ExperienceReward>(0);
			wearer.armorClass = std::make_unique<ArmorClass>(BARE_ARMOR_CLASS);
			wearer.healthPool = std::make_unique<HealthPool>(20);
			wearer.set_strength(CARRIER_STRENGTH);
			wearer.set_dexterity(NEUTRAL_DEXTERITY);
			ctx.playerOwner = &mock.player;
		}

		Player& player() { return *mock.player; }

		// Makes the item from its record, type identified, carries it and puts
		// it on the way the inventory's use does.
		Item& wear(std::string_view key)
		{
			auto made = ItemCreator::create(key, player().position, ctx);
			made->identify_type();
			Item* carried = made.get();
			[[maybe_unused]] const auto added = InventoryOperations::add_item(
				player().inventoryData,
				std::move(made));
			[[maybe_unused]] const bool used =
				use_item(*carried->behavior, *carried, player(), ctx);
			return *carried;
		}

		int armor_class() { return player().get_armor_class(); }

		// Whether both ring slots hold a ring, so a pair test measures a pair.
		bool is_ringed_on_both_hands()
		{
			return player().get_equipped_item(EquipmentSlot::RIGHT_RING) !=
				nullptr &&
				player().get_equipped_item(EquipmentSlot::LEFT_RING) != nullptr;
		}
	};

	TEST_F(RingOfProtectionTest, APlusOneRingBettersArmourClassByOne)
	{
		wear("ring_of_protection_plus_1");

		EXPECT_EQ(armor_class(), PLUS_ONE_ARMOR_CLASS);
	}

	TEST_F(RingOfProtectionTest, APlusTwoRingBettersArmourClassByTwo)
	{
		wear("ring_of_protection_plus_2");

		EXPECT_EQ(armor_class(), PLUS_TWO_ARMOR_CLASS);
	}

	// Worn second, the weaker ring must not displace the stronger.
	TEST_F(
		RingOfProtectionTest,
		OfTwoRingsOnlyTheStrongerCountsWhenItIsWornFirst)
	{
		const Item& strong = wear("ring_of_protection_plus_2");
		wear("ring_of_protection_plus_1");
		ASSERT_TRUE(is_ringed_on_both_hands());

		const ArmorClassBreakdown breakdown =
			player().armorClass->update(player(), ctx);

		EXPECT_EQ(armor_class(), PLUS_TWO_ARMOR_CLASS);
		EXPECT_EQ(breakdown.ring.name, strong.get_name());
		EXPECT_EQ(
			breakdown.ring.bonus,
			PLUS_TWO_ARMOR_CLASS - BARE_ARMOR_CLASS);
	}

	// Worn first, the weaker ring must not keep its place once the stronger is
	// on.
	TEST_F(
		RingOfProtectionTest,
		OfTwoRingsOnlyTheStrongerCountsWhenItIsWornSecond)
	{
		wear("ring_of_protection_plus_1");
		const Item& strong = wear("ring_of_protection_plus_2");
		ASSERT_TRUE(is_ringed_on_both_hands());

		const ArmorClassBreakdown breakdown =
			player().armorClass->update(player(), ctx);

		EXPECT_EQ(armor_class(), PLUS_TWO_ARMOR_CLASS);
		EXPECT_EQ(breakdown.ring.name, strong.get_name());
		EXPECT_EQ(
			breakdown.ring.bonus,
			PLUS_TWO_ARMOR_CLASS - BARE_ARMOR_CLASS);
	}

	// A ring's plus counts only when its effect is protection: fire resistance
	// +1 leaves armour class alone, as putting it on recomputes it.
	TEST_F(RingOfProtectionTest, ARingThatIsNotProtectionLeavesArmourClassAlone)
	{
		wear("ring_of_fire_resistance");
		ASSERT_TRUE(
			player().get_equipped_item(EquipmentSlot::RIGHT_RING) != nullptr ||
			player().get_equipped_item(EquipmentSlot::LEFT_RING) != nullptr)
			<< "the ring of fire resistance did not go on";

		EXPECT_EQ(armor_class(), BARE_ARMOR_CLASS);
	}

	// The breakdown names only the items that counted, so it leaves out a ring
	// of fire resistance.
	TEST_F(RingOfProtectionTest, ARingThatIsNotProtectionIsNotNamedAsCounting)
	{
		wear("ring_of_fire_resistance");

		const ArmorClassBreakdown breakdown =
			player().armorClass->update(player(), ctx);
		EXPECT_TRUE(breakdown.ring.name.empty())
			<< "a ring that is not protection was named as counting";
	}

} // namespace CppRL
