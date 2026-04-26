// file: EquipmentStatBonusTest.cpp
// Verifies that equipping and unequipping items with stat bonuses correctly
// applies and removes bonuses from Player attributes.
// Tests both ItemEnhancement stat bonuses (strength_bonus, dexterity_bonus)
// and behavior-based bonuses (JewelryAmulet, Gauntlets, Girdle).

#include "src/Actor/Actor.h"
#include "src/Actor/Creature.h"
#include "src/Actor/Destructible.h"
#include "src/ActorTypes/Player.h"
#include "src/Actor/Item.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Systems/DataManager.h"
#include "src/Items/ItemIdentification.h"
#include "src/Items/MagicalItemEffects.h"
#include "tests/mocks/MockGameContext.h"
#include <Core/GameContext.h>
#include <gtest/gtest.h>
#include <memory>
#include <Utils/Vector2D.h>

// ============================================================================
// TEST FIXTURE: EquipmentStatBonusTest
// ============================================================================

class EquipmentStatBonusTest : public ::testing::Test
{
protected:
    MockGameContext mock;
    GameContext ctx;
    DataManager data_manager;
    std::unique_ptr<Player> player;

    void SetUp() override
    {
        data_manager.load_all_data(mock.messages);

        player = std::make_unique<Player>(Vector2D{ 0, 0 });
        player->experienceReward = std::make_unique<ExperienceReward>(0);
        player->set_dr(5);
        player->set_thaco(20);
        player->armorClass = std::make_unique<ArmorClass>(10);
        player->destructible = std::make_unique<Destructible>(100, 10, std::make_unique<PlayerDeathHandler>());

        ctx = mock.to_game_context();
        ctx.player = player.get();
    }

    std::unique_ptr<Item> create_item_with_strength_bonus(int bonus)
    {
        auto item = std::make_unique<Item>(Vector2D{}, ActorData{});
        item->actorData.name = "Test Item +" + std::to_string(bonus) + " STR";
        item->actorData.tile = TileRef{};
        item->enhancement.strength_bonus = bonus;
        item->itemClass = ItemClass::SWORD;
        item->behavior = Weapon{};
        return item;
    }

    std::unique_ptr<Item> create_item_with_dexterity_bonus(int bonus)
    {
        auto item = std::make_unique<Item>(Vector2D{}, ActorData{});
        item->actorData.name = "Test Item +" + std::to_string(bonus) + " DEX";
        item->actorData.tile = TileRef{};
        item->enhancement.dexterity_bonus = bonus;
        item->itemClass = ItemClass::SWORD;
        item->behavior = Weapon{};
        return item;
    }

    std::unique_ptr<Item> create_item_with_both_bonuses(int str_bonus, int dex_bonus)
    {
        auto item = std::make_unique<Item>(Vector2D{}, ActorData{});
        item->actorData.name = "Test Item +" + std::to_string(str_bonus) + " STR +" + std::to_string(dex_bonus) + " DEX";
        item->actorData.tile = TileRef{};
        item->enhancement.strength_bonus = str_bonus;
        item->enhancement.dexterity_bonus = dex_bonus;
        item->itemClass = ItemClass::SWORD;
        item->behavior = Weapon{};
        return item;
    }

    std::unique_ptr<Item> create_amulet_with_bonus(int str_bonus, int dex_bonus)
    {
        auto item = std::make_unique<Item>(Vector2D{}, ActorData{});
        item->actorData.name = "Test Amulet";
        item->actorData.tile = TileRef{};
        item->itemClass = ItemClass::AMULET;
        JewelryAmulet amulet;
        amulet.strBonus = str_bonus;
        amulet.dexBonus = dex_bonus;
        amulet.isSetMode = false;  // Additive mode, not set mode
        item->behavior = amulet;
        return item;
    }
};

// ============================================================================
// ItemEnhancement Stat Bonuses (from items.json)
// ============================================================================

TEST_F(EquipmentStatBonusTest, EquipItem_WithStrengthBonus_IncreasesStrength)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(2);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 2);
}

TEST_F(EquipmentStatBonusTest, UnequipItem_WithStrengthBonus_DecreasesStrength)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(3);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    int boosted_str = player->get_strength();
    EXPECT_EQ(boosted_str, original_str + 3);
    
    player->unequip_item(EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str);
}

TEST_F(EquipmentStatBonusTest, EquipItem_WithDexterityBonus_IncreaseDexterity)
{
    int original_dex = player->get_dexterity();
    auto item = create_item_with_dexterity_bonus(2);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_dexterity(), original_dex + 2);
}

TEST_F(EquipmentStatBonusTest, UnequipItem_WithDexterityBonus_DecreaseDexterity)
{
    int original_dex = player->get_dexterity();
    auto item = create_item_with_dexterity_bonus(2);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    int boosted_dex = player->get_dexterity();
    EXPECT_EQ(boosted_dex, original_dex + 2);
    
    player->unequip_item(EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_dexterity(), original_dex);
}

TEST_F(EquipmentStatBonusTest, EquipItem_WithBothBonuses_AppliesBoth)
{
    int original_str = player->get_strength();
    int original_dex = player->get_dexterity();
    auto item = create_item_with_both_bonuses(2, 1);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 2);
    EXPECT_EQ(player->get_dexterity(), original_dex + 1);
}

TEST_F(EquipmentStatBonusTest, EquipItem_WithNegativeBonus_DecreasesAttribute)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(-1);  // Cursed item
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str - 1);
}

TEST_F(EquipmentStatBonusTest, EquipAndUnequip_RestoredToOriginal)
{
    int original_str = player->get_strength();
    int original_dex = player->get_dexterity();
    auto item = create_item_with_both_bonuses(3, 2);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    EXPECT_EQ(player->get_strength(), original_str + 3);
    EXPECT_EQ(player->get_dexterity(), original_dex + 2);
    
    player->unequip_item(EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str);
    EXPECT_EQ(player->get_dexterity(), original_dex);
}

// ============================================================================
// Behavior-based Stat Bonuses (JewelryAmulet, Gauntlets, Girdle)
// ============================================================================

TEST_F(EquipmentStatBonusTest, EquipAmulet_WithStrengthBonus_IncreasesStrength)
{
    int original_str = player->get_strength();
    auto item = create_amulet_with_bonus(2, 0);
    
    player->equip_item(std::move(item), EquipmentSlot::NECK, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 2);
}

TEST_F(EquipmentStatBonusTest, EquipAmulet_WithDexterityBonus_IncreaseDexterity)
{
    int original_dex = player->get_dexterity();
    auto item = create_amulet_with_bonus(0, 1);
    
    player->equip_item(std::move(item), EquipmentSlot::NECK, ctx);
    
    EXPECT_EQ(player->get_dexterity(), original_dex + 1);
}

TEST_F(EquipmentStatBonusTest, EquipAmulet_WithBothBonuses_AppliesBoth)
{
    int original_str = player->get_strength();
    int original_dex = player->get_dexterity();
    auto item = create_amulet_with_bonus(1, 2);
    
    player->equip_item(std::move(item), EquipmentSlot::NECK, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 1);
    EXPECT_EQ(player->get_dexterity(), original_dex + 2);
}

TEST_F(EquipmentStatBonusTest, UnequipAmulet_WithBonus_RestoresOriginalStats)
{
    int original_str = player->get_strength();
    auto item = create_amulet_with_bonus(3, 0);
    
    player->equip_item(std::move(item), EquipmentSlot::NECK, ctx);
    EXPECT_EQ(player->get_strength(), original_str + 3);
    
    player->unequip_item(EquipmentSlot::NECK, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(EquipmentStatBonusTest, EquipItem_WithZeroBonus_NoChange)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(0);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str);
}

TEST_F(EquipmentStatBonusTest, EquipMultipleItems_BonusesStack)
{
    int original_str = player->get_strength();
    auto item1 = create_item_with_strength_bonus(2);
    auto item2 = create_item_with_strength_bonus(1);
    
    player->equip_item(std::move(item1), EquipmentSlot::RIGHT_HAND, ctx);
    player->equip_item(std::move(item2), EquipmentSlot::LEFT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 3);
}

TEST_F(EquipmentStatBonusTest, UnequipOneOfTwoItems_RemovesOnlyOne)
{
    int original_str = player->get_strength();
    auto item1 = create_item_with_strength_bonus(2);
    auto item2 = create_item_with_strength_bonus(1);
    
    player->equip_item(std::move(item1), EquipmentSlot::RIGHT_HAND, ctx);
    player->equip_item(std::move(item2), EquipmentSlot::LEFT_HAND, ctx);
    
    player->unequip_item(EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 1);
}

TEST_F(EquipmentStatBonusTest, LargePositiveBonus_Applied)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(10);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str + 10);
}

TEST_F(EquipmentStatBonusTest, LargeNegativeBonus_Applied)
{
    int original_str = player->get_strength();
    auto item = create_item_with_strength_bonus(-5);
    
    player->equip_item(std::move(item), EquipmentSlot::RIGHT_HAND, ctx);
    
    EXPECT_EQ(player->get_strength(), original_str - 5);
}
