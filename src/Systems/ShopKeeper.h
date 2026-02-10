#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../Actor/InventoryData.h"
#include "../Persistent/Persistent.h"

struct GameContext;

enum class ShopType
{
    WEAPON_SHOP,
    ARMOR_SHOP,
    GENERAL_STORE,
    POTION_SHOP,
    SCROLL_SHOP,
    ADVENTURING_GEAR
};

enum class ShopQuality
{
    POOR,      // 70% base price
    AVERAGE,   // 100% base price
    GOOD,      // 130% base price
    EXCELLENT  // 160% base price
};

// Forward declarations
struct GameContext;
class Item;
class Creature;

class ShopKeeper : public Persistent
{
public:
    ShopType shop_type;
    ShopQuality shop_quality;
    std::string shop_name;
    InventoryData shop_inventory{50}; // Shop's items for sale
    int markup_percent{120};    // Buy price percentage
    int sellback_percent{60};   // Sell price percentage

    ShopKeeper(ShopType type, ShopQuality quality);

    // Default constructor for deserialization
    ShopKeeper();

    // Serialization - required by Persistent interface
    void load(const json& j) override;
    void save(json& j) override;

    // Factory method for deserialization
    static std::unique_ptr<ShopKeeper> create(const json& j);
    
    // Trading operations - simplified for now
    bool can_buy_item(const Item& item) const { return true; } // Accept all items for now
    int get_buy_price(const Item& item) const;
    int get_sell_price(const Item& item) const;
    
    // Inventory management
    void generate_initial_inventory(GameContext& ctx);
    
    // Transaction handling
    bool process_player_purchase(GameContext& ctx, Item& item, Creature& player);
    bool process_player_sale(GameContext& ctx, Item& item, Creature& player);
    
    // Static utility function for creating random shopkeepers
    static std::unique_ptr<ShopKeeper> create_random_shopkeeper();
    
private:
    void generate_shop_name();

    // Random item generation methods
    std::unique_ptr<Item> generate_random_item_by_type(GameContext& ctx);
    std::unique_ptr<Item> generate_random_weapon(GameContext& ctx);
    std::unique_ptr<Item> generate_random_armor(GameContext& ctx);
    std::unique_ptr<Item> generate_random_potion(GameContext& ctx);
    std::unique_ptr<Item> generate_random_scroll(GameContext& ctx);
    std::unique_ptr<Item> generate_random_misc_item(GameContext& ctx);
};
