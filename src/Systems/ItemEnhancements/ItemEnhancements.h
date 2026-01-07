 #pragma once

#include <string>
#include <vector>
#include <memory>

enum class PrefixType
{
    NONE,
    // Weapon prefixes
    SHARP,          // +1 damage
    KEEN,           // +2 damage
    MASTERWORK,     // +1 to hit
    BLESSED,        // +1 to hit and damage
    FLAMING,        // Fire damage
    FROST,          // Cold damage
    SHOCK,          // Lightning damage
    // Armor prefixes
    REINFORCED,     // +1 AC
    STUDDED,        // +1 AC
    ELVEN,          // +2 AC, lighter
    DWARVEN,        // +3 AC, heavier
    MAGICAL,        // +1 AC, magical
    // Universal prefixes
    CURSED,         // Negative effects
    ANCIENT,        // Increased value
    RUSTED,         // Decreased effectiveness
    CRACKED         // Reduced durability
};

enum class SuffixType
{
    NONE,
    // Combat suffixes
    OF_SLAYING,     // +3 damage
    OF_ACCURACY,    // +2 to hit
    OF_PROTECTION,  // +2 AC
    OF_POWER,       // +1 all stats
    // Resistance suffixes
    OF_FIRE_RESISTANCE,
    OF_COLD_RESISTANCE,
    OF_LIGHTNING_RESISTANCE,
    OF_POISON_RESISTANCE,
    // Special suffixes
    OF_SPEED,       // Movement bonus
    OF_STEALTH,     // Stealth bonus
    OF_MAGIC,       // Mana bonus
    OF_HEALTH,      // HP bonus
    OF_THE_EAGLE,   // Dexterity bonus
    OF_THE_BEAR,    // Strength bonus
    OF_THE_OWL,     // Intelligence bonus
    // Negative suffixes
    OF_WEAKNESS,    // Stat penalty
    OF_SLOWNESS,    // Movement penalty
    OF_BRITTLENESS  // Durability penalty
};

struct ItemEnhancement
{
    PrefixType prefix = PrefixType::NONE;
    SuffixType suffix = SuffixType::NONE;
    
    // Modifier values
    int damage_bonus = 0;
    int to_hit_bonus = 0;
    int ac_bonus = 0;
    int strength_bonus = 0;
    int dexterity_bonus = 0;
    int intelligence_bonus = 0;
    int hp_bonus = 0;
    int mana_bonus = 0;
    int speed_bonus = 0;
    int stealth_bonus = 0;
    
    // Resistances (0-100%)
    int fire_resistance = 0;
    int cold_resistance = 0;
    int lightning_resistance = 0;
    int poison_resistance = 0;
    
    // Special properties
    bool is_cursed = false;
    bool is_blessed = false;
    bool is_magical = false;

    // Numeric enhancement level for traditional "+X" display
    int enhancement_level = 0;

    // Value modifier (percentage)
    int value_modifier = 100; // 100% = no change
    
    // Generate enhancement name
    std::string get_prefix_name() const;
    std::string get_suffix_name() const;
    std::string get_full_name(const std::string& base_name) const;
    
    // Apply enhancement effects
    void apply_enhancement_effects();
    
    // Enhancement generation
    static ItemEnhancement generate_random_enhancement(bool allow_magical = true);
    static ItemEnhancement generate_weapon_enhancement();
    static ItemEnhancement generate_armor_enhancement();
    static ItemEnhancement generate_potion_enhancement();
    static ItemEnhancement generate_scroll_enhancement();
    
    // Rarity-based generation
    static ItemEnhancement generate_by_rarity(int rarity_level); // 1-5
    
private:
    static PrefixType get_random_weapon_prefix();
    static PrefixType get_random_armor_prefix();
    static PrefixType get_random_universal_prefix();
    static SuffixType get_random_combat_suffix();
    static SuffixType get_random_resistance_suffix();
    static SuffixType get_random_special_suffix();
};
