#include "ShopKeeper.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Factories/ItemCreator.h"  // SINGLE SOURCE OF TRUTH
#include <algorithm>

using namespace InventoryOperations;

ShopKeeper::ShopKeeper(ShopType type, ShopQuality quality) 
    : shop_type(type), shop_quality(quality)
{
    // Set pricing based on quality
    switch (quality) 
    {
        case ShopQuality::POOR:
            markup_percent = 110;
            sellback_percent = 50;
            break;
        case ShopQuality::AVERAGE:
            markup_percent = 120;
            sellback_percent = 60;
            break;
        case ShopQuality::GOOD:
            markup_percent = 130;
            sellback_percent = 70;
            break;
        case ShopQuality::EXCELLENT:
            markup_percent = 150;
            sellback_percent = 80;
            break;
    }

    generate_shop_name();
    // Note: generate_initial_inventory(ctx) must be called separately after construction
}

int ShopKeeper::get_buy_price(const Item& item) const
{
    int base_price = item.value > 0 ? item.value : 10;
    return (base_price * markup_percent) / 100;
}

int ShopKeeper::get_sell_price(const Item& item) const
{
    int base_price = item.value > 0 ? item.value : 10;
    return (base_price * sellback_percent) / 100;
}

void ShopKeeper::generate_initial_inventory(GameContext& ctx)
{
    shop_inventory.items.clear();

    // Generate 3-7 random items based on shop type
    int item_count = ctx.dice->roll(3, 7);

    for (int i = 0; i < item_count; i++)
    {
        std::unique_ptr<Item> item = generate_random_item_by_type(ctx);
        if (item)
        {
            add_item(shop_inventory, std::move(item));
        }
    }
}

std::unique_ptr<Item> ShopKeeper::generate_random_item_by_type(GameContext& ctx)
{
    switch (shop_type)
    {
        case ShopType::WEAPON_SHOP:
            return generate_random_weapon(ctx);
        case ShopType::ARMOR_SHOP:
            return generate_random_armor(ctx);
        case ShopType::POTION_SHOP:
            return generate_random_potion(ctx);
        case ShopType::SCROLL_SHOP:
            return generate_random_scroll(ctx);
        case ShopType::GENERAL_STORE:
            switch (ctx.dice->roll(0, 3))
            {
                case 0: return generate_random_weapon(ctx);
                case 1: return generate_random_armor(ctx);
                case 2: return generate_random_potion(ctx);
                case 3: return generate_random_scroll(ctx);
            }
            break;
        default:
            return generate_random_misc_item(ctx);
    }
    return nullptr;
}

std::unique_ptr<Item> ShopKeeper::generate_random_weapon(GameContext& ctx)
{
    const int level = ctx.level_manager->get_dungeon_level();
    auto item = ItemCreator::create_random_of_category("weapon", { 0, 0 }, ctx, level);

    if (item && ctx.dice->roll(1, 100) <= 40)
        item->generate_random_enhancement(true);

    return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_armor(GameContext& ctx)
{
    const int level = ctx.level_manager->get_dungeon_level();
    auto item = ItemCreator::create_random_of_category("armor", { 0, 0 }, ctx, level);

    if (item && ctx.dice->roll(1, 100) <= 35)
        item->generate_random_enhancement(true);

    return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_potion(GameContext& ctx)
{
    const int level = ctx.level_manager->get_dungeon_level();
    return ItemCreator::create_random_of_category("potion", { 0, 0 }, ctx, level);
}

std::unique_ptr<Item> ShopKeeper::generate_random_scroll(GameContext& ctx)
{
    const int level = ctx.level_manager->get_dungeon_level();
    return ItemCreator::create_random_of_category("scroll", { 0, 0 }, ctx, level);
}

std::unique_ptr<Item> ShopKeeper::generate_random_misc_item(GameContext& ctx)
{
    Vector2D shop_pos{ 0, 0 };
    const int level = ctx.level_manager->get_dungeon_level();

    std::unique_ptr<Item> item;
    const int category = ctx.dice->roll(0, 3);
    switch (category)
    {
        case 0: item = ItemCreator::create_random_of_category("weapon", shop_pos, ctx, level); break;
        case 1: item = ItemCreator::create_random_of_category("armor", shop_pos, ctx, level); break;
        case 2: item = ItemCreator::create_random_of_category("potion", shop_pos, ctx, level); break;
        case 3: item = ItemCreator::create(ItemId::FOOD_RATION, shop_pos); break;
    }

    // Apply price variation (±5)
    const int baseValue = item->base_value;
    item->base_value = baseValue + ctx.dice->roll(-5, 5);
    item->base_value = std::max(1, item->base_value);
    item->value = item->base_value;

    // 15% chance for enhancement (only for equipment)
    if (item->is_weapon() || item->is_armor())
    {
        if (ctx.dice->roll(1, 100) <= 15)
        {
            item->generate_random_enhancement(false);
        }
    }

    return item;
}

bool ShopKeeper::process_player_purchase(GameContext& ctx, Item& item, Creature& player)
{
    int price = get_buy_price(item);

    if (player.get_gold() < price)
    {
        ctx.message_system->message(WHITE_RED_PAIR, "You don't have enough gold!", true);
        return false;
    }

    if (is_inventory_full(player.inventory_data))
    {
        ctx.message_system->message(WHITE_RED_PAIR, "Your inventory is full!", true);
        return false;
    }

    player.adjust_gold(-price);

    auto player_item = std::make_unique<Item>(item.position, item.actorData);
    player_item->value = get_sell_price(item);
    player_item->base_value = item.base_value;
    player_item->enhancement = item.enhancement; // Copy enhancement
    player_item->itemClass = item.itemClass;

    // Copy pickable component if it exists
    if (item.pickable)
    {
        // Create a copy of the pickable component
        json pickableJson;
        item.pickable->save(pickableJson);
        player_item->pickable = Pickable::create(pickableJson);
    }

    add_item(player.inventory_data, std::move(player_item));

    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "You bought ");
    ctx.message_system->append_message_part(YELLOW_BLACK_PAIR, item.get_name()); // Use enhanced name
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " for ");
    ctx.message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(price));
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " gold.");
    ctx.message_system->finalize_message();

    return true;
}

bool ShopKeeper::process_player_sale(GameContext& ctx, Item& item, Creature& player)
{
    int price = get_sell_price(item);

    player.adjust_gold(price);

    auto removed_item = remove_item(player.inventory_data, item);
    if (removed_item.has_value())
    {
        if (!is_inventory_full(shop_inventory))
        {
            (*removed_item)->value = get_buy_price(*(*removed_item));
            add_item(shop_inventory, std::move(*removed_item));
        }
    }

    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "You sold ");
    ctx.message_system->append_message_part(YELLOW_BLACK_PAIR, item.get_name()); // Use enhanced name
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " for ");
    ctx.message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(price));
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " gold.");
    ctx.message_system->finalize_message();

    return true;
}

void ShopKeeper::generate_shop_name()
{
    std::vector<std::string> weapon_names = {
        "The Sharp Edge", "Blades & Bludgeons", "Steel & Iron", "The Armory", 
        "Warrior's Arsenal", "The Forge", "Sword & Shield", "Battle Ready"
    };
    
    std::vector<std::string> armor_names = {
        "Ironclad Armory", "Plate & Mail", "The Defender", "Armor & Protection",
        "Steel Defense", "Guardian's Gear", "The Shield Wall", "Heavy Metal"
    };
    
    std::vector<std::string> potion_names = {
        "Mystic Brews", "The Alchemy Shop", "Bubbling Cauldron", "Elixir Emporium",
        "Potion Master", "The Brew House", "Magical Mixtures", "Liquid Magic"
    };
    
    std::vector<std::string> scroll_names = {
        "Arcane Scrolls", "The Scriptorium", "Magical Manuscripts", "Spell Scrolls",
        "The Magic Word", "Enchanted Texts", "Scroll & Quill", "Ancient Writings"
    };
    
    std::vector<std::string> general_names = {
        "General Store", "The Trading Post", "Odds & Ends", "Everything Shop",
        "The Merchant's Den", "All Things", "Trade & Barter", "The Bazaar"
    };
    
    switch (shop_type) 
    {
        case ShopType::WEAPON_SHOP:
            shop_name = weapon_names[rand() % weapon_names.size()];
            break;
        case ShopType::ARMOR_SHOP:
            shop_name = armor_names[rand() % armor_names.size()];
            break;
        case ShopType::POTION_SHOP:
            shop_name = potion_names[rand() % potion_names.size()];
            break;
        case ShopType::SCROLL_SHOP:
            shop_name = scroll_names[rand() % scroll_names.size()];
            break;
        case ShopType::GENERAL_STORE:
            shop_name = general_names[rand() % general_names.size()];
            break;
        default:
            shop_name = "Shop";
            break;
            }
}

// Static utility function for creating random shopkeepers
std::unique_ptr<ShopKeeper> ShopKeeper::create_random_shopkeeper()
{
    // Random shop type selection with weighted probabilities
    int typeRoll = rand() % 100;
    ShopType randomType;
    
    if (typeRoll < 25)
        randomType = ShopType::WEAPON_SHOP;      // 25% chance
    else if (typeRoll < 45) 
        randomType = ShopType::ARMOR_SHOP;       // 20% chance
    else if (typeRoll < 65)
        randomType = ShopType::POTION_SHOP;      // 20% chance
    else if (typeRoll < 80)
        randomType = ShopType::SCROLL_SHOP;      // 15% chance
    else if (typeRoll < 90)
        randomType = ShopType::GENERAL_STORE;    // 10% chance
    else
        randomType = ShopType::ADVENTURING_GEAR; // 10% chance
    
    // Random quality selection with weighted probabilities
    int qualityRoll = rand() % 100;
    ShopQuality randomQuality;
    
    if (qualityRoll < 15)
        randomQuality = ShopQuality::POOR;       // 15% chance
    else if (qualityRoll < 65)
        randomQuality = ShopQuality::AVERAGE;    // 50% chance
    else if (qualityRoll < 90)
        randomQuality = ShopQuality::GOOD;       // 25% chance
    else
        randomQuality = ShopQuality::EXCELLENT;  // 10% chance
    
    return std::make_unique<ShopKeeper>(randomType, randomQuality);
}

// Default constructor for deserialization
ShopKeeper::ShopKeeper()
    : shop_type(ShopType::GENERAL_STORE)
    , shop_quality(ShopQuality::AVERAGE)
    , markup_percent(120)
    , sellback_percent(60)
{
    // Don't generate inventory - will be loaded from save
}

void ShopKeeper::save(json& j)
{
    j["shop_type"] = static_cast<int>(shop_type);
    j["shop_quality"] = static_cast<int>(shop_quality);
    j["shop_name"] = shop_name;
    j["markup_percent"] = markup_percent;
    j["sellback_percent"] = sellback_percent;

    // Save shop inventory
    json inventoryJson;
    save_inventory(shop_inventory, inventoryJson);
    j["shop_inventory"] = inventoryJson;
}

void ShopKeeper::load(const json& j)
{
    shop_type = static_cast<ShopType>(j.at("shop_type").get<int>());
    shop_quality = static_cast<ShopQuality>(j.at("shop_quality").get<int>());
    shop_name = j.at("shop_name").get<std::string>();
    markup_percent = j.at("markup_percent").get<int>();
    sellback_percent = j.at("sellback_percent").get<int>();

    // Load shop inventory
    if (j.contains("shop_inventory"))
    {
        shop_inventory = InventoryData(50);
        load_inventory(shop_inventory, j["shop_inventory"]);
    }
}

std::unique_ptr<ShopKeeper> ShopKeeper::create(const json& j)
{
    auto shopkeeper = std::make_unique<ShopKeeper>();
    shopkeeper->load(j);
    return shopkeeper;
}
