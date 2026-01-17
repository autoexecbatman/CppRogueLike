#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/Systems/ShopKeeper.h"
#include "src/Actor/InventoryOperations.h"
#include "src/Factories/ItemCreator.h"

using json = nlohmann::json;
using namespace InventoryOperations;

// ============================================================================
// SHOPKEEPER SERIALIZATION TESTS
// Ensures shop inventory persists across save/load
// ============================================================================

class ShopKeeperSerializationTest : public ::testing::Test {
protected:
    std::unique_ptr<ShopKeeper> create_test_shop() {
        auto shop = std::make_unique<ShopKeeper>(ShopType::WEAPON_SHOP, ShopQuality::GOOD);
        return shop;
    }
};

TEST_F(ShopKeeperSerializationTest, BasicFields_SaveLoad_RoundTrip) {
    ShopKeeper original(ShopType::ARMOR_SHOP, ShopQuality::EXCELLENT);

    json j;
    original.save(j);

    auto loaded = ShopKeeper::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->shop_type, ShopType::ARMOR_SHOP);
    EXPECT_EQ(loaded->shop_quality, ShopQuality::EXCELLENT);
    EXPECT_EQ(loaded->shop_name, original.shop_name);
    EXPECT_EQ(loaded->markup_percent, original.markup_percent);
    EXPECT_EQ(loaded->sellback_percent, original.sellback_percent);
}

TEST_F(ShopKeeperSerializationTest, Inventory_Preserved) {
    ShopKeeper original(ShopType::WEAPON_SHOP, ShopQuality::AVERAGE);

    // Shop should have generated 3-7 items
    size_t original_count = get_item_count(original.shop_inventory);
    ASSERT_GE(original_count, 3) << "Shop should generate at least 3 items";
    ASSERT_LE(original_count, 7) << "Shop should generate at most 7 items";

    json j;
    original.save(j);

    auto loaded = ShopKeeper::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(get_item_count(loaded->shop_inventory), original_count);
}

TEST_F(ShopKeeperSerializationTest, AllShopTypes_SaveLoad) {
    std::vector<ShopType> types = {
        ShopType::WEAPON_SHOP,
        ShopType::ARMOR_SHOP,
        ShopType::GENERAL_STORE,
        ShopType::POTION_SHOP,
        ShopType::SCROLL_SHOP,
        ShopType::ADVENTURING_GEAR
    };

    for (ShopType type : types) {
        ShopKeeper original(type, ShopQuality::AVERAGE);

        json j;
        original.save(j);

        auto loaded = ShopKeeper::create(j);

        ASSERT_NE(loaded, nullptr) << "Failed to load shop type " << static_cast<int>(type);
        EXPECT_EQ(loaded->shop_type, type);
    }
}

TEST_F(ShopKeeperSerializationTest, AllQualities_SaveLoad) {
    std::vector<ShopQuality> qualities = {
        ShopQuality::POOR,
        ShopQuality::AVERAGE,
        ShopQuality::GOOD,
        ShopQuality::EXCELLENT
    };

    for (ShopQuality quality : qualities) {
        ShopKeeper original(ShopType::GENERAL_STORE, quality);

        json j;
        original.save(j);

        auto loaded = ShopKeeper::create(j);

        ASSERT_NE(loaded, nullptr) << "Failed to load shop quality " << static_cast<int>(quality);
        EXPECT_EQ(loaded->shop_quality, quality);
    }
}

TEST_F(ShopKeeperSerializationTest, EmptyInventory_HandledGracefully) {
    ShopKeeper original(ShopType::WEAPON_SHOP, ShopQuality::AVERAGE);

    // Clear the inventory
    original.shop_inventory.items.clear();

    json j;
    original.save(j);

    auto loaded = ShopKeeper::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(is_inventory_empty(loaded->shop_inventory));
}

TEST_F(ShopKeeperSerializationTest, CustomPricing_Preserved) {
    ShopKeeper original(ShopType::GENERAL_STORE, ShopQuality::AVERAGE);
    original.markup_percent = 200;
    original.sellback_percent = 30;

    json j;
    original.save(j);

    auto loaded = ShopKeeper::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->markup_percent, 200);
    EXPECT_EQ(loaded->sellback_percent, 30);
}
