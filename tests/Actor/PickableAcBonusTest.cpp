// file: PickableAcBonusTest.cpp
// Verifies get_item_ac_bonus dispatches correctly after the if constexpr refactor.
// The old code had 15 individual overloads; the new code has 4 explicit cases + catch-all.
// Every alternative in ItemBehavior is exercised here.
#include <gtest/gtest.h>

#include <fstream>

#include <nlohmann/json.hpp>

#include "src/MagicalItemEffects.h"
#include "src/Paths.h"
#include "src/Pickable.h"

// ============================================================================
// AC-granting types -- must return correct non-zero values
// ============================================================================

TEST(PickableAcBonus, Armor_ReturnsArmorClass)
{
    const ItemBehavior behavior = Armor{ 5 };
    EXPECT_EQ(get_item_ac_bonus(behavior), 5);
}

TEST(PickableAcBonus, Armor_ZeroArmorClass_ReturnsZero)
{
    const ItemBehavior behavior = Armor{ 0 };
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Shield_ReturnsMinusOne)
{
	// AD&D 2e: shield grants +1 AC improvement, encoded as -1 (lower AC = better)
	const ItemBehavior behavior = Shield{};
	EXPECT_EQ(get_item_ac_bonus(behavior), -1);
}

// A helm of brilliance "is armor of +2 value" (Dungeon Master's Guide, PDF page 968 of
// the 2e archive), and its worth is the item's own effectBonus rather than a number the
// code keeps beside it.
TEST(PickableAcBonus, MagicalHelm_Brilliance_IsWorthItsOwnBonus)
{
	const ItemBehavior asTheBookRatesIt = MagicalHelm{ MagicalEffect::BRILLIANCE, 2 };
	EXPECT_EQ(get_item_ac_bonus(asTheBookRatesIt), -2);
}

TEST(PickableAcBonus, MagicalHelm_Brilliance_ReadsTheBonusRatherThanAConstant)
{
	// A different bonus must give a different answer, or the helm is carrying a number
	// the data cannot change - which is how it came to be worth +4.
	const ItemBehavior enchantedFurther = MagicalHelm{ MagicalEffect::BRILLIANCE, 3 };
	EXPECT_EQ(get_item_ac_bonus(enchantedFurther), -3);
}

TEST(PickableAcBonus, MagicalHelm_Brilliance_MatchesTheItemTheGameShips)
{
	// The record the game actually loads says +2, so the two agree end to end.
	std::ifstream file(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(file.is_open()) << "items.json did not open";
	nlohmann::json items;
	file >> items;

	const int shippedBonus = items.at("helm_of_brilliance").at("effectBonus").get<int>();
	EXPECT_EQ(shippedBonus, 2) << "the shipped helm is no longer the book's +2 helm";

	const ItemBehavior behavior = MagicalHelm{ MagicalEffect::BRILLIANCE, shippedBonus };
	EXPECT_EQ(get_item_ac_bonus(behavior), -2);
}

TEST(PickableAcBonus, MagicalHelm_Protection_ReturnsNegativeBonus)
{
    const ItemBehavior behavior = MagicalHelm{ MagicalEffect::PROTECTION, 3 };
    EXPECT_EQ(get_item_ac_bonus(behavior), -3);
}

TEST(PickableAcBonus, MagicalHelm_NoEffect_ReturnsZero)
{
    const ItemBehavior behavior = MagicalHelm{ MagicalEffect::NONE, 0 };
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, MagicalRing_Protection_ReturnsMinusOne)
{
    const ItemBehavior behavior = MagicalRing{ MagicalEffect::PROTECTION, 0 };
    EXPECT_EQ(get_item_ac_bonus(behavior), -1);
}

TEST(PickableAcBonus, MagicalRing_NoEffect_ReturnsZero)
{
    const ItemBehavior behavior = MagicalRing{ MagicalEffect::NONE, 0 };
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

// ============================================================================
// Non-equipment catch-all -- all must return 0
// ============================================================================

TEST(PickableAcBonus, Consumable_ReturnsZero)
{
    const ItemBehavior behavior = Consumable{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Weapon_ReturnsZero)
{
    const ItemBehavior behavior = Weapon{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, TargetedScroll_ReturnsZero)
{
    const ItemBehavior behavior = TargetedScroll{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Teleporter_ReturnsZero)
{
    const ItemBehavior behavior = Teleporter{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Gold_ReturnsZero)
{
    const ItemBehavior behavior = Gold{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Food_ReturnsZero)
{
    const ItemBehavior behavior = Food{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, CorpseFood_ReturnsZero)
{
    const ItemBehavior behavior = CorpseFood{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, JewelryAmulet_ReturnsZero)
{
    const ItemBehavior behavior = JewelryAmulet{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Gauntlets_ReturnsZero)
{
    const ItemBehavior behavior = Gauntlets{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Girdle_ReturnsZero)
{
    const ItemBehavior behavior = Girdle{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}

TEST(PickableAcBonus, Amulet_ReturnsZero)
{
    const ItemBehavior behavior = Amulet{};
    EXPECT_EQ(get_item_ac_bonus(behavior), 0);
}
