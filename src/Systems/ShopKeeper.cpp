#include "ShopKeeper.h"
#include "../Game.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Confuser.h"
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
    
    struct WeaponData 
    {
        std::string name;
        char symbol;
        int base_price;
    };
    
    static const WeaponData weapons[] = 
    {
        {"Iron Sword", '/', 15},
        {"Steel Sword", '/', 25},
        {"Dagger", ')', 2},
        {"War Hammer", 'T', 8},
        {"Battle Axe", 'r', 12},
        {"Long Bow", '}', 75},
        {"Short Sword", '/', 10},
        {"Great Sword", '/', 50},
        {"Mace", 'T', 6}
    };
    
    const WeaponData& weapon = weapons[rand() % (sizeof(weapons) / sizeof(weapons[0]))];
    
    auto item = std::make_unique<Item>(shop_pos, ActorData{weapon.symbol, weapon.name, WHITE_BLACK_PAIR});
    item->base_value = weapon.base_price + (rand() % 10) - 5;
    item->base_value = std::max(1, item->base_value);
    item->value = item->base_value;
    item->initialize_item_type_from_name();
    
    // Add appropriate pickable component based on weapon type
    if (weapon.name.find("Dagger") != std::string::npos)
        item->pickable = std::make_unique<Dagger>();
    else if (weapon.name.find("Short Sword") != std::string::npos)
        item->pickable = std::make_unique<ShortSword>();
    else if (weapon.name.find("Long Bow") != std::string::npos)
        item->pickable = std::make_unique<Longbow>();
    else if (weapon.name.find("Great Sword") != std::string::npos)
        item->pickable = std::make_unique<Greatsword>();
    else if (weapon.name.find("Battle Axe") != std::string::npos)
        item->pickable = std::make_unique<BattleAxe>();
    else if (weapon.name.find("War Hammer") != std::string::npos)
        item->pickable = std::make_unique<WarHammer>();
    else // Default to LongSword for swords
        item->pickable = std::make_unique<LongSword>();
    
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
        // Add armor pickable components for other armor types
        if (armor.name == "Leather Armor")
            item->pickable = std::make_unique<LeatherArmor>();
        else if (armor.name == "Chain Mail")
            item->pickable = std::make_unique<ChainMail>();
        else if (armor.name == "Plate Mail")
            item->pickable = std::make_unique<PlateMail>();
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
    Vector2D shop_pos{0, 0};
    
    struct PotionData 
    {
        std::string name;
        int base_price;
        Pickable::PickableType type;
        int min_value;
        int max_value;
    };
    
    static const PotionData potions[] = 
    {
        {"Healing Potion", 50, Pickable::PickableType::HEALING_POTION, 15, 35},
        {"Mana Potion", 40, Pickable::PickableType::MANA_POTION, 10, 25},
        {"Strength Potion", 75, Pickable::PickableType::STRENGTH_POTION, 2, 4},
        {"Speed Potion", 60, Pickable::PickableType::SPEED_POTION, 1, 2},
        {"Poison Antidote", 30, Pickable::PickableType::POISON_ANTIDOTE, 1, 1},
        {"Fire Resistance Potion", 80, Pickable::PickableType::FIRE_RESISTANCE_POTION, 30, 70},
        {"Invisibility Potion", 200, Pickable::PickableType::INVISIBILITY_POTION, 20, 50}
    };
    
    const PotionData& potion = potions[rand() % (sizeof(potions) / sizeof(potions[0]))];
    
    auto item = std::make_unique<Item>(shop_pos, ActorData{'!', potion.name, RED_BLACK_PAIR});
    item->base_value = potion.base_price + (rand() % 20) - 10;
    item->base_value = std::max(5, item->base_value);
    item->value = item->base_value;
    item->initialize_item_type_from_name();
    
    // Create appropriate potion pickable with random stats
    switch (potion.type)
    {
        case Pickable::PickableType::HEALING_POTION:
        {
            int heal_amount = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            item->pickable = std::make_unique<HealingPotion>(heal_amount);
            break;
        }
        case Pickable::PickableType::MANA_POTION:
        {
            int mana_amount = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            item->pickable = std::make_unique<ManaPotion>(mana_amount);
            break;
        }
        case Pickable::PickableType::STRENGTH_POTION:
        {
            int strength_bonus = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            int duration = 80 + (rand() % 40); // 80-120 turns
            item->pickable = std::make_unique<StrengthPotion>(strength_bonus, duration);
            break;
        }
        case Pickable::PickableType::SPEED_POTION:
        {
            int speed_bonus = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            int duration = 40 + (rand() % 30); // 40-70 turns
            item->pickable = std::make_unique<SpeedPotion>(speed_bonus, duration);
            break;
        }
        case Pickable::PickableType::POISON_ANTIDOTE:
        {
            item->pickable = std::make_unique<PoisonAntidote>();
            break;
        }
        case Pickable::PickableType::FIRE_RESISTANCE_POTION:
        {
            int resistance = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            int duration = 150 + (rand() % 100); // 150-250 turns
            item->pickable = std::make_unique<FireResistancePotion>(resistance, duration);
            break;
        }
        case Pickable::PickableType::INVISIBILITY_POTION:
        {
            int duration = potion.min_value + (rand() % (potion.max_value - potion.min_value + 1));
            item->pickable = std::make_unique<InvisibilityPotion>(duration);
            break;
        }
        default:
            item->pickable = std::make_unique<HealingPotion>();
            break;
    }
    
    // 15% chance for enhanced potions (better stats)
    if (rand() % 100 < 15)
    {
        item->generate_random_enhancement(false);
        // Boost the potion's effectiveness for enhanced versions
        item->value = static_cast<int>(item->value * 1.3f);
    }
    
    return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_scroll()
{
    Vector2D shop_pos{0, 0};
    
    struct ScrollData 
    {
        std::string name;
        int base_price;
        Pickable::PickableType type;
        int min_value;
        int max_value;
    };
    
    static const ScrollData scrolls[] = 
    {
        {"Scroll of Fireball", 375, Pickable::PickableType::FIREBALL, 0, 0},
        {"Scroll of Lightning", 375, Pickable::PickableType::LIGHTNING_BOLT, 0, 0},
        {"Scroll of Confusion", 100, Pickable::PickableType::CONFUSER, 0, 0},
        {"Scroll of Identify", 50, Pickable::PickableType::SCROLL_IDENTIFY, 0, 0},
        {"Scroll of Teleport", 150, Pickable::PickableType::SCROLL_TELEPORT, 8, 15},
        {"Scroll of Magic Mapping", 300, Pickable::PickableType::SCROLL_MAGIC_MAPPING, 20, 35},
        {"Scroll of Enchantment", 500, Pickable::PickableType::SCROLL_ENCHANTMENT, 1, 3}
    };
    
    const ScrollData& scroll = scrolls[rand() % (sizeof(scrolls) / sizeof(scrolls[0]))];
    
    auto item = std::make_unique<Item>(shop_pos, ActorData{'?', scroll.name, YELLOW_BLACK_PAIR});
    item->base_value = scroll.base_price + (rand() % 50) - 25;
    item->base_value = std::max(10, item->base_value);
    item->value = item->base_value;
    item->initialize_item_type_from_name();
    
    // Create appropriate scroll pickable with random stats where applicable
    switch (scroll.type)
    {
        case Pickable::PickableType::FIREBALL:
        {
            int range = 5 + (rand() % 3); // 5-7 range
            int damage = 20 + (rand() % 15); // 20-35 damage
            item->pickable = std::make_unique<Fireball>(range, damage);
            break;
        }
        case Pickable::PickableType::LIGHTNING_BOLT:
        {
            int range = 8 + (rand() % 4); // 8-11 range
            int damage = 30 + (rand() % 20); // 30-50 damage
            item->pickable = std::make_unique<LightningBolt>(range, damage);
            break;
        }
        case Pickable::PickableType::CONFUSER:
        {
            int range = 8 + (rand() % 2); // 8-9 range
            int nb_turns = 10 + (rand() % 10); // 10-20 turns
            item->pickable = std::make_unique<Confuser>(range, nb_turns);
            break;
        }
        case Pickable::PickableType::SCROLL_IDENTIFY:
        {
            item->pickable = std::make_unique<ScrollIdentify>();
            break;
        }
        case Pickable::PickableType::SCROLL_TELEPORT:
        {
            int range = scroll.min_value + (rand() % (scroll.max_value - scroll.min_value + 1));
            item->pickable = std::make_unique<ScrollTeleport>(range);
            break;
        }
        case Pickable::PickableType::SCROLL_MAGIC_MAPPING:
        {
            int radius = scroll.min_value + (rand() % (scroll.max_value - scroll.min_value + 1));
            item->pickable = std::make_unique<ScrollMagicMapping>(radius);
            break;
        }
        case Pickable::PickableType::SCROLL_ENCHANTMENT:
        {
            int bonus = scroll.min_value + (rand() % (scroll.max_value - scroll.min_value + 1));
            item->pickable = std::make_unique<ScrollEnchantment>(bonus);
            break;
        }
        default:
            item->pickable = std::make_unique<ScrollIdentify>();
            break;
    }
    
    // Scrolls are inherently magical, no additional enhancements needed
    
    return item;
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

bool ShopKeeper::process_player_purchase(Item& item, Creature& player)
{
    int price = get_buy_price(item);
    
    if (player.get_gold() < price) 
    {
        game.message(WHITE_RED_PAIR, "You don't have enough gold!", true);
        return false;
    }
    
    if (is_inventory_full(player.inventory_data)) 
    {
        game.message(WHITE_RED_PAIR, "Your inventory is full!", true);
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
    
    game.append_message_part(WHITE_BLACK_PAIR, "You bought ");
    game.append_message_part(YELLOW_BLACK_PAIR, item.get_name()); // Use enhanced name
    game.append_message_part(WHITE_BLACK_PAIR, " for ");
    game.append_message_part(YELLOW_BLACK_PAIR, std::to_string(price));
    game.append_message_part(WHITE_BLACK_PAIR, " gold.");
    game.finalize_message();
    
    return true;
}

bool ShopKeeper::process_player_sale(Item& item, Creature& player)
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
    
    game.append_message_part(WHITE_BLACK_PAIR, "You sold ");
    game.append_message_part(YELLOW_BLACK_PAIR, item.get_name()); // Use enhanced name
    game.append_message_part(WHITE_BLACK_PAIR, " for ");
    game.append_message_part(YELLOW_BLACK_PAIR, std::to_string(price));
    game.append_message_part(WHITE_BLACK_PAIR, " gold.");
    game.finalize_message();
    
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
