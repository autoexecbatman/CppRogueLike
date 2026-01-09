Here's a self-contained unit test file for the `ItemClassification.cpp` module, following all the rules and guidelines provided:

cpp
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>
#include "ItemClassification.h" // Assuming this is where ItemClassification.h is located

// Include the source file to test its functions
#include "E:/repo/C++RogueLike/src/Items/ItemClassification.cpp"

void test_get_category() {
    using namespace ItemClassificationUtils;
    
    assert(get_category(ItemClass::DAGGER) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::SHORT_SWORD) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::LONG_SWORD) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::GREAT_SWORD) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::BATTLE_AXE) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::GREAT_AXE) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::WAR_HAMMER) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::MACE) == ItemCategory::WEAPON);
    assert(get_category(ItemClass::STAFF) == ItemCategory::WEAPON);
    
    assert(get_category(ItemClass::LEATHER_ARMOR) == ItemCategory::ARMOR);
    assert(get_category(ItemClass::CHAIN_MAIL) == ItemCategory::ARMOR);
    assert(get_category(ItemClass::PLATE_MAIL) == ItemCategory::ARMOR);
    
    assert(get_category(ItemClass::SMALL_SHIELD) == ItemCategory::SHIELD);
    assert(get_category(ItemClass::MEDIUM_SHIELD) == ItemCategory::SHIELD);
    assert(get_category(ItemClass::LARGE_SHIELD) == ItemCategory::SHIELD);
    
    assert(get_category(ItemClass::HEALTH_POTION) == ItemCategory::CONSUMABLE);
    assert(get_category(ItemClass::MANA_POTION) == ItemCategory::CONSUMABLE);
    assert(get_category(ItemClass::FOOD_RATION) == ItemCategory::CONSUMABLE);
    assert(get_category(ItemClass::BREAD) == ItemCategory::CONSUMABLE);
    assert(get_category(ItemClass::MEAT) == ItemCategory::CONSUMABLE);
    assert(get_category(ItemClass::FRUIT) == ItemCategory::CONSUMABLE);
    
    assert(get_category(ItemClass::SCROLL_LIGHTNING) == ItemCategory::SCROLL);
    assert(get_category(ItemClass::SCROLL_FIREBALL) == ItemCategory::SCROLL);
    assert(get_category(ItemClass::SCROLL_CONFUSION) == ItemCategory::SCROLL);
    assert(get_category(ItemClass::SCROLL_TELEPORT) == ItemCategory::SCROLL);
    
    assert(get_category(ItemClass::AMULET) == ItemCategory::JEWELRY);
    assert(get_category(ItemClass::RING) == ItemCategory::JEWELRY);
    
    assert(get_category(ItemClass::GOLD) == ItemCategory::TREASURE);
    assert(get_category(ItemClass::GEM) == ItemCategory::TREASURE);
    
    assert(get_category(ItemClass::TORCH) == ItemCategory::TOOL);
    assert(get_category(ItemClass::ROPE) == ItemCategory::TOOL);
    assert(get_category(ItemClass::LOCKPICK) == ItemCategory::TOOL);
    
    assert(get_category(ItemClass::AMULET_OF_YENDOR) == ItemCategory::QUEST_ITEM);
    
    assert(get_category(static_cast<ItemClass>(99)) == ItemCategory::UNKNOWN); // Unknown item class
    
    std::cout << "test_get_category passed!" << std::endl;
}

void test_get_display_name() {
    using namespace ItemClassificationUtils;
    
    assert(get_display_name(ItemClass::DAGGER) == "dagger");
    assert(get_display_name(ItemClass::SHORT_SWORD) == "short sword");
    assert(get_display_name(ItemClass::LONG_SWORD) == "long sword");
    assert(get_display_name(ItemClass::GREAT_SWORD) == "great sword");
    assert(get_display_name(ItemClass::BATTLE_AXE) == "battle axe");
    assert(get_display_name(ItemClass::GREAT_AXE) == "great axe");
    assert(get_display_name(ItemClass::WAR_HAMMER) == "war hammer");
    assert(get_display_name(ItemClass::MACE) == "mace");
    assert(get_display_name(ItemClass::STAFF) == "staff");
    
    assert(get_display_name(ItemClass::LONG_BOW) == "long bow");
    assert(get_display_name(ItemClass::SHORT_BOW) == "short bow");
    assert(get_display_name(ItemClass::CROSSBOW) == "crossbow");
    
    assert(get_display_name(ItemClass::LEATHER_ARMOR) == "leather armor");
    assert(get_display_name(ItemClass::CHAIN_MAIL) == "chain mail");
    assert(get_display_name(ItemClass::PLATE_MAIL) == "plate mail");
    
    assert(get_display_name(ItemClass::SMALL_SHIELD) == "small shield");
    assert(get_display_name(ItemClass::MEDIUM_SHIELD) == "shield");
    assert(get_display_name(ItemClass::LARGE_SHIELD) == "large shield");
    
    assert(get_display_name(ItemClass::HEALTH_POTION) == "health potion");
    assert(get_display_name(ItemClass::MANA_POTION) == "mana potion");
    assert(get_display_name(ItemClass::FOOD_RATION) == "food ration");
    assert(get_display_name(ItemClass::BREAD) == "bread");
    assert(get_display_name(ItemClass::MEAT) == "meat");
    assert(get_display_name(ItemClass::FRUIT) == "fruit");
    
    assert(get_display_name(ItemClass::SCROLL_LIGHTNING) == "lightning bolt scroll");
    assert(get_display_name(ItemClass::SCROLL_FIREBALL) == "fireball scroll");
    assert(get_display_name(ItemClass::SCROLL_CONFUSION) == "confusion scroll");
    assert(get_display_name(ItemClass::SCROLL_TELEPORT) == "teleport scroll");
    
    assert(get_display_name(ItemClass::AMULET) == "amulet");
    assert(get_display_name(ItemClass::RING) == "ring");
    
    assert(get_display_name(ItemClass::GOLD) == "gold");
    assert(get_display_name(ItemClass::GEM) == "gem");
    
    assert(get_display_name(ItemClass::TORCH) == "torch");
    assert(get_display_name(ItemClass::ROPE) == "rope");
    assert(get_display_name(ItemClass::LOCKPICK) == "lockpick");
    
    assert(get_display_name(ItemClass::AMULET_OF_YENDOR) == "Amulet of Yendor");
    
    assert(get_display_name(static_cast<ItemClass>(99)) == "unknown item");
    
    std::cout << "test_get_display_name passed!" << std::endl;
}

void test_can_equip_to_right_hand() {
    using namespace ItemClassificationUtils;
    
    assert(can_equip_to_right_hand(ItemClass::DAGGER));
    assert(can_equip_to_right_hand(ItemClass::SHORT_SWORD));
    assert(can_equip_to_right_hand(ItemClass::LONG_SWORD));
    assert(can_equip_to_right_hand(ItemClass::GREAT_SWORD));
    assert(can_equip_to_right_hand(ItemClass::BATTLE_AXE));
    assert(can_equip_to_right_hand(ItemClass::GREAT_AXE));
    assert(can_equip_to_right_hand(ItemClass::WAR_HAMMER));
    assert(can_equip_to_right_hand(ItemClass::MACE));
    assert(can_equip_to_right_hand(ItemClass::STAFF));
    assert(can_equip_to_right_hand(ItemClass::SMALL_SHIELD));
    assert(can_equip_to_right_hand(ItemClass::MEDIUM_SHIELD));
    assert(can_equip_to_right_hand(ItemClass::LARGE_SHIELD));
    
    assert(!can_equip_to_right_hand(ItemClass::LEATHER_ARMOR));
    assert(!can_equip_to_right_hand(ItemClass::CHAIN_MAIL));
    assert(!can_equip_to_right_hand(ItemClass::PLATE_MAIL));
    
    assert(!can_equip_to_right_hand(ItemClass::HEALTH_POTION));
    assert(!can_equip_to_right_hand(ItemClass::MANA_POTION));
    assert(!can_equip_to_right_hand(ItemClass::FOOD_RATION));
    assert(!can_equip_to_right_hand(ItemClass::BREAD));
    assert(!can_equip_to_right_hand(ItemClass::MEAT));
    assert(!can_equip_to_right_hand(ItemClass::FRUIT));
    
    assert(!can_equip_to_right_hand(ItemClass::SCROLL_LIGHTNING));
    assert(!can_equip_to_right_hand(ItemClass::SCROLL_FIREBALL));
    assert(!can_equip_to_right_hand(ItemClass::SCROLL_CONFUSION));
    assert(!can_equip_to_right_hand(ItemClass::SCROLL_TELEPORT));
    
    assert(!can_equip_to_right_hand(ItemClass::AMULET));
    assert(!can_equip_to_right_hand(ItemClass::RING));
    
    assert(!can_equip_to_right_hand(ItemClass::GOLD));
    assert(!can_equip_to_right_hand(ItemClass::GEM));
    
    assert(!can_equip_to_right_hand(ItemClass::TORCH));
    assert(!can_equip_to_right_hand(ItemClass::ROPE));
    assert(!can_equip_to_right_hand(ItemClass::LOCKPICK));
    
    assert(!can_equip_to_right_hand(ItemClass::AMULET_OF_YENDOR));
    
    std::cout << "test_can_equip_to_right_hand passed!" << std::endl;
}

void test_can_equip_to_left_hand() {
    using namespace ItemClassificationUtils;
    
    assert(can_equip_to_left_hand(ItemClass::DAGGER) == false);
    assert(can_equip_to_left_hand(ItemClass::SHORT_SWORD) == false);
    assert(can_equip_to_left_hand(ItemClass::LONG_SWORD) == false);
    assert(can_equip_to_left_hand(ItemClass::GREAT_SWORD) == false);
    assert(can_equip_to_left_hand(ItemClass::BATTLE_AXE) == false);
    assert(can_equip_to_left_hand(ItemClass::GREAT_AXE) == false);
    assert(can_equip_to_left_hand(ItemClass::WAR_HAMMER) == false);
    assert(can_equip_to_left_hand(ItemClass::MACE) == false);
    assert(can_equip_to_left_hand(ItemClass::STAFF) == false);
    
    assert(can_equip_to_left_hand(ItemClass::SMALL_SHIELD));
    assert(can_equip_to_left_hand(ItemClass::MEDIUM_SHIELD));
    assert(can_equip_to_left_hand(ItemClass::LARGE_SHIELD) == false);
    
    assert(!can_equip_to_left_hand(ItemClass::LEATHER_ARMOR));
    assert(!can_equip_to_left_hand(ItemClass::CHAIN_MAIL));
    assert(!can_equip_to_left_hand(ItemClass::PLATE_MAIL));
    
    assert(!can_equip_to_left_hand(ItemClass::HEALTH_POTION));
    assert(!can_equip_to_left_hand(ItemClass::MANA_POTION));
    assert(!can_equip_to_left_hand(ItemClass::FOOD_RATION));
    assert(!can_equip_to_left_hand(ItemClass::BREAD));
    assert(!can_equip_to_left_hand(ItemClass::MEAT));
    assert(!can_equip_to_left_hand(ItemClass::FRUIT));
    
    assert(!can_equip_to_left_hand(ItemClass::SCROLL_LIGHTNING));
    assert(!can_equip_to_left_hand(ItemClass::SCROLL_FIREBALL));
    assert(!can_equip_to_left_hand(ItemClass::SCROLL_CONFUSION));
    assert(!can_equip_to_left_hand(ItemClass::SCROLL_TELEPORT));
    
    assert(!can_equip_to_left_hand(ItemClass::AMULET));
    assert(!can_equip_to_left_hand(ItemClass::RING));
    
    assert(!can_equip_to_left_hand(ItemClass::GOLD));
    assert(!can_equip_to_left_hand(ItemClass::GEM));
    
    assert(!can_equip_to_left_hand(ItemClass::TORCH));
    assert(!can_equip_to_left_hand(ItemClass::ROPE));
    assert(!can_equip_to_left_hand(ItemClass::LOCKPICK));
    
    assert(!can_equip_to_left_hand(ItemClass::AMULET_OF_YENDOR));
    
    std::cout << "test_can_equip_to_left_hand passed!" << std::endl;
}

void test_is_two_handed_weapon() {
    using namespace ItemClassificationUtils;
    
    assert(is_two_handed_weapon(ItemClass::GREAT_SWORD));
    assert(is_two_handed_weapon(ItemClass::GREAT_AXE));
    assert(is_two_handed_weapon(ItemClass::LONG_BOW));
    assert(is_two_handed_weapon(ItemClass::SHORT_BOW));
    assert(is_two_handed_weapon(ItemClass::CROSSBOW));
    assert(is_two_handed_weapon(ItemClass::STAFF));
    
    assert(!is_two_handed_weapon(ItemClass::DAGGER));
    assert(!is_two_handed_weapon(ItemClass::SHORT_SWORD));
    assert(!is_two_handed_weapon(ItemClass::LONG_SWORD));
    assert(!is_two_handed_weapon(ItemClass::BATTLE_AXE));
    assert(!is_two_handed_weapon(ItemClass::WAR_HAMMER));
    assert(!is_two_handed_weapon(ItemClass::MACE));
    
    std::cout << "test_is_two_handed_weapon passed!" << std::endl;
}

void test_is_ranged_weapon() {
    using namespace ItemClassificationUtils;
    
    assert(is_ranged_weapon(ItemClass::LONG_BOW));
    assert(is_ranged_weapon(ItemClass::SHORT_BOW));
    assert(is_ranged_weapon(ItemClass::CROSSBOW));
    
    assert(!is_ranged_weapon(ItemClass::DAGGER));
    assert(!is_ranged_weapon(ItemClass::SHORT_SWORD));
    assert(!is_ranged_weapon(ItemClass::LONG_SWORD));
    assert(!is_ranged_weapon(ItemClass::GREAT_SWORD));
    assert(!is_ranged_weapon(ItemClass::BATTLE_AXE));
    assert(!is_ranged_weapon(ItemClass::GREAT_AXE));
    assert(!is_ranged_weapon(ItemClass::WAR_HAMMER));
    assert(!is_ranged_weapon(ItemClass::MACE));
    assert(!is_ranged_weapon(ItemClass::STAFF));
    
    std::cout << "test_is_ranged_weapon passed!" << std::endl;
}

void test_from_string() {
    using namespace ItemClassificationUtils;
    
    assert(from_string("dagger") == ItemClass::DAGGER);
    assert(from_string("short_sword") == ItemClass::SHORT_SWORD);
    assert(from_string("long_sword") == ItemClass::LONG_SWORD);
    assert(from_string("great_sword") == ItemClass::GREAT_SWORD);
    assert(from_string("battle_axe") == ItemClass::BATTLE_AXE);
    assert(from_string("great_axe") == ItemClass::GREAT_AXE);
    assert(from_string("war_hammer") == ItemClass::WAR_HAMMER);
    assert(from_string("mace") == ItemClass::MACE);
    assert(from_string("staff") == ItemClass::STAFF);
    
    assert(from_string("long_bow") == ItemClass::LONG_BOW);
    assert(from_string("short_bow") == ItemClass::SHORT_BOW);
    assert(from_string("crossbow") == ItemClass::CROSSBOW);
    
    assert(from_string("leather_armor") == ItemClass::LEATHER_ARMOR);
    assert(from_string("chain_mail") == ItemClass::CHAIN_MAIL);
    assert(from_string("plate_mail") == ItemClass::PLATE_MAIL);
    
    assert(from_string("small_shield") == ItemClass::SMALL_SHIELD);
    assert(from_string("shield") == ItemClass::MEDIUM_SHIELD);
    assert(from_string("large_shield") == ItemClass::LARGE_SHIELD);
    
    assert(from_string("health_potion") == ItemClass::HEALTH_POTION);
    assert(from_string("mana_potion") == ItemClass::MANA_POTION);
    assert(from_string("food_ration") == ItemClass::FOOD_RATION);
    assert(from_string("bread") == ItemClass::BREAD);
    assert(from_string("meat") == ItemClass::MEAT);
    assert(from_string("fruit") == ItemClass::FRUIT);
    
    assert(from_string("scroll_lightning") == ItemClass::SCROLL_LIGHTNING);
    assert(from_string("scroll_fireball") == ItemClass::SCROLL_FIREBALL);
    assert(from_string("scroll_confusion") == ItemClass::SCROLL_CONFUSION);
    assert(from_string("scroll_teleport") == ItemClass::SCROLL_TELEPORT);
    
    assert(from_string("amulet") == ItemClass::AMULET);
    assert(from_string("ring") == ItemClass::RING);
    
    assert(from_string("gold") == ItemClass::GOLD);
    assert(from_string("gem") == ItemClass::GEM);
    
    assert(from_string("torch") == ItemClass::TORCH);
    assert(from_string("rope") == ItemClass::ROPE);
    assert(from_string("lockpick") == ItemClass::LOCKPICK);
    
    assert(from_string("amulet_of_yendor") == ItemClass::AMULET_OF_YENDOR);
    
    assert(from_string("unknown_item") == ItemClass::UNKNOWN);
    
    std::cout << "test_from_string passed!" << std::endl;
}

void test_get_weapon_size() {
    using namespace ItemClassificationUtils;
    
    assert(get_weapon_size(ItemClass::DAGGER) == WeaponSize::TINY);
    assert(get_weapon_size(ItemClass::SHORT_SWORD) == WeaponSize::SMALL);
    assert(get_weapon_size(ItemClass::LONG_SWORD) == WeaponSize::MEDIUM);
    assert(get_weapon_size(ItemClass::GREAT_SWORD) == WeaponSize::LARGE);
    assert(get_weapon_size(ItemClass::BATTLE_AXE) == WeaponSize::MEDIUM);
    assert(get_weapon_size(ItemClass::GREAT_AXE) == WeaponSize::LARGE);
    assert(get_weapon_size(ItemClass::WAR_HAMMER) == WeaponSize::LARGE);
    assert(get_weapon_size(ItemClass::MACE) == WeaponSize::MEDIUM);
    assert(get_weapon_size(ItemClass::STAFF) == WeaponSize::LARGE);
    
    assert(get_weapon_size(ItemClass::LONG_BOW) == WeaponSize::LARGE);
    assert(get_weapon_size(ItemClass::SHORT_BOW) == WeaponSize::SMALL);
    assert(get_weapon_size(ItemClass::CROSSBOW) == WeaponSize::MEDIUM);
    
    std::cout << "test_get_weapon_size passed!" << std::endl;
}

int main() {
    test_get_category();
    test_get_display_name();
    test_can_equip_to_right_hand();
    test_can_equip_to_left_hand();
    test_is_two_handed_weapon();
    test_is_ranged_weapon();
    test_from_string();
    test_get_weapon_size();
    
    return 0;
}