#include "ShopKeeper.h"
#include "../Core/GameContext.h"
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
    generate_initial_inventory();
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

void ShopKeeper::generate_initial_inventory()
{
    shop_inventory.items.clear();
    
    // Generate 3-7 random items based on shop type
    int item_count = 3 + (rand() % 5);
    
    for (int i = 0; i < item_count; i++)
    {
        std::unique_ptr<Item> item = generate_random_item_by_type();
        if (item)
        {
            add_item(shop_inventory, std::move(item));
        }
    }
}

std::unique_ptr<Item> ShopKeeper::generate_random_item_by_type()
{
    switch (shop_type)
    {
        case ShopType::WEAPON_SHOP:
            return generate_random_weapon();
        case ShopType::ARMOR_SHOP:
            return generate_random_armor();
        case ShopType::POTION_SHOP:
            return generate_random_potion();
        case ShopType::SCROLL_SHOP:
            return generate_random_scroll();
        case ShopType::GENERAL_STORE:
            switch (rand() % 4)
            {
                case 0: return generate_random_weapon();
                case 1: return generate_random_armor();
                case 2: return generate_random_potion();
                case 3: return generate_random_scroll();
            }
            break;
        default:
            return generate_random_misc_item();
    }
    return nullptr;
}

std::unique_ptr<Item> ShopKeeper::generate_random_weapon()
{
    Vector2D shop_pos{0, 0};
    
    static const ItemClass available_weapons[] = {
        ItemClass::DAGGER,
        ItemClass::SHORT_SWORD,
        ItemClass::LONG_SWORD,
        ItemClass::STAFF,
        ItemClass::BATTLE_AXE,
        ItemClass::WAR_HAMMER,
        ItemClass::MACE,
        ItemClass::GREAT_SWORD,
        ItemClass::LONG_BOW
    };

    ItemClass weaponClass = available_weapons[rand() % (sizeof(available_weapons) / sizeof(available_weapons[0]))];

    // Use ItemCreator factory methods for consistency
    auto item = ItemCreator::create_weapon_by_class(shop_pos, weaponClass);
    
    // 40% chance for enhancement
    if (rand() % 100 < 40)
    {
        item->generate_random_enhancement(true);
    }
    
    return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_armor()
{
    Vector2D shop_pos{0, 0};
    
    struct ArmorData 
    {
        std::string name;
        char symbol;
        int base_price;
    };
    
    static const ArmorData armors[] = 
    {
        {"Leather Armor", '[', 5},
        {"Chain Mail", '[', 75},
        {"Plate Mail", '[', 600},
        {"Small Shield", ')', 10},
        {"Large Shield", ')', 25},
        {"Studded Leather", '[', 20},
        {"Scale Mail", '[', 120},
        {"Ring Mail", '[', 30}
    };
    
    const ArmorData& armor = armors[rand() % (sizeof(armors) / sizeof(armors[0]))];
    
    auto item = std::make_unique<Item>(shop_pos, ActorData{armor.symbol, armor.name, CYAN_BLACK_PAIR});
    item->base_value = armor.base_price + (rand() % (armor.base_price / 5 + 1));
    item->value = item->base_value;
    item->initialize_item_type_from_name();
    
    // Add appropriate pickable component based on item type
    if (armor.name.find("Shield") != std::string::npos)
    {
        item->pickable = std::make_unique<Shield>();
    }
    else
    {
        // Create armor directly using ItemCreator
        if (armor.name == "Leather Armor")
        {
            auto armor_item = ItemCreator::create_leather_armor(Vector2D{0, 0});
            item->pickable = std::move(armor_item->pickable);
        }
        else if (armor.name == "Chain Mail")
        {
            auto armor_item = ItemCreator::create_chain_mail(Vector2D{0, 0});
            item->pickable = std::move(armor_item->pickable);
        }
        else if (armor.name == "Plate Mail")
        {
            auto armor_item = ItemCreator::create_plate_mail(Vector2D{0, 0});
            item->pickable = std::move(armor_item->pickable);
        }
    }
    
    // 35% chance for enhancement
    if (rand() % 100 < 35)
    {
        item->generate_random_enhancement(true);
    }
    
    return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_potion()
{
    // USE UNIFIED ITEM CREATION - SINGLE SOURCE OF TRUTH
    return ItemCreator::create_health_potion(Vector2D{0, 0});
}

std::unique_ptr<Item> ShopKeeper::generate_random_scroll()
{
    // USE UNIFIED ITEM CREATION - SINGLE SOURCE OF TRUTH
    return ItemCreator::create_scroll_lightning(Vector2D{0, 0});
}

std::unique_ptr<Item> ShopKeeper::generate_random_misc_item()
{
    Vector2D shop_pos{0, 0};
    
    struct MiscData 
    {
        std::string name;
        char symbol;
        int base_price;
    };
    
    static const MiscData misc[] = 
    {
        {"Torch", '~', 2},
        {"Rope", '%', 5},
        {"Rations", '%', 3},
        {"Thieves Tools", '(', 25},
        {"Holy Symbol", '*', 15},
        {"Gems", '*', 100},
        {"Gold Ring", '=', 50}
    };
    
    const MiscData& item_data = misc[rand() % (sizeof(misc) / sizeof(misc[0]))];
    
    auto item = std::make_unique<Item>(shop_pos, ActorData{item_data.symbol, item_data.name, WHITE_BLACK_PAIR});
    item->base_value = item_data.base_price + (rand() % 10) - 5;
    item->base_value = std::max(1, item->base_value);
    item->value = item->base_value;
    item->initialize_item_type_from_name();
    
    // 15% chance for enhancement
    if (rand() % 100 < 15)
    {
        item->generate_random_enhancement(false);
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
