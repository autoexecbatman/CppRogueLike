// file: PickableAcBonusTest.cpp
// Verifies get_item_ac_bonus dispatches correctly after the if constexpr refactor.
// The old code had 15 individual overloads; the new code has 4 explicit cases + catch-all.
// Every alternative in ItemBehavior is exercised here.
#include <gtest/gtest.h>

#include "src/Actor/Pickable.h"
#include "src/Items/MagicalItemEffects.h"

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

TEST(PickableAcBonus, MagicalHelm_Brilliance_ReturnsMinus4)
{
    const ItemBehavior behavior = MagicalHelm{ MagicalEffect::BRILLIANCE, 0 };
    EXPECT_EQ(get_item_ac_bonus(behavior), -4);
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
