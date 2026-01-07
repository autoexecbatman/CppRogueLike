#include "ItemEnhancements.h"
#include <random>

// Enhancement name getters
std::string ItemEnhancement::get_prefix_name() const
{
    switch (prefix)
    {
        case PrefixType::SHARP: return "Sharp";
        case PrefixType::KEEN: return "Keen";
        case PrefixType::MASTERWORK: return "Masterwork";
        case PrefixType::BLESSED: return "Blessed";
        case PrefixType::FLAMING: return "Flaming";
        case PrefixType::FROST: return "Frost";
        case PrefixType::SHOCK: return "Shock";
        case PrefixType::REINFORCED: return "Reinforced";
        case PrefixType::STUDDED: return "Studded";
        case PrefixType::ELVEN: return "Elven";
        case PrefixType::DWARVEN: return "Dwarven";
        case PrefixType::MAGICAL: return "Magical";
        case PrefixType::CURSED: return "Cursed";
        case PrefixType::ANCIENT: return "Ancient";
        case PrefixType::RUSTED: return "Rusted";
        case PrefixType::CRACKED: return "Cracked";
        default: return "";
    }
}

std::string ItemEnhancement::get_suffix_name() const
{
    switch (suffix)
    {
        case SuffixType::OF_SLAYING: return "of Slaying";
        case SuffixType::OF_ACCURACY: return "of Accuracy";
        case SuffixType::OF_PROTECTION: return "of Protection";
        case SuffixType::OF_POWER: return "of Power";
        case SuffixType::OF_FIRE_RESISTANCE: return "of Fire Resistance";
        case SuffixType::OF_COLD_RESISTANCE: return "of Cold Resistance";
        case SuffixType::OF_LIGHTNING_RESISTANCE: return "of Lightning Resistance";
        case SuffixType::OF_POISON_RESISTANCE: return "of Poison Resistance";
        case SuffixType::OF_SPEED: return "of Speed";
        case SuffixType::OF_STEALTH: return "of Stealth";
        case SuffixType::OF_MAGIC: return "of Magic";
        case SuffixType::OF_HEALTH: return "of Health";
        case SuffixType::OF_THE_EAGLE: return "of the Eagle";
        case SuffixType::OF_THE_BEAR: return "of the Bear";
        case SuffixType::OF_THE_OWL: return "of the Owl";
        case SuffixType::OF_WEAKNESS: return "of Weakness";
        case SuffixType::OF_SLOWNESS: return "of Slowness";
        case SuffixType::OF_BRITTLENESS: return "of Brittleness";
        default: return "";
    }
}

std::string ItemEnhancement::get_full_name(const std::string& base_name) const
{
    std::string name = "";

    // Handle traditional "+X" enhancement level first
    if (enhancement_level > 0)
    {
        name = base_name + " +" + std::to_string(enhancement_level);
        return name;
    }

    // Handle prefix/suffix system
    if (prefix != PrefixType::NONE)
    {
        name += get_prefix_name() + " ";
    }

    name += base_name;

    if (suffix != SuffixType::NONE)
    {
        name += " " + get_suffix_name();
    }

    return name;
}

void ItemEnhancement::apply_enhancement_effects()
{
    // Apply prefix effects
    switch (prefix)
    {
        case PrefixType::SHARP:
            damage_bonus += 1;
            value_modifier += 50;
            break;
        case PrefixType::KEEN:
            damage_bonus += 2;
            value_modifier += 100;
            break;
        case PrefixType::MASTERWORK:
            to_hit_bonus += 1;
            value_modifier += 75;
            break;
        case PrefixType::BLESSED:
            to_hit_bonus += 1;
            damage_bonus += 1;
            is_blessed = true;
            value_modifier += 150;
            break;
        case PrefixType::FLAMING:
            damage_bonus += 2;
            fire_resistance = 25;
            is_magical = true;
            value_modifier += 200;
            break;
        case PrefixType::FROST:
            damage_bonus += 2;
            cold_resistance = 25;
            is_magical = true;
            value_modifier += 200;
            break;
        case PrefixType::SHOCK:
            damage_bonus += 2;
            lightning_resistance = 25;
            is_magical = true;
            value_modifier += 200;
            break;
        case PrefixType::REINFORCED:
            ac_bonus += 1;
            value_modifier += 60;
            break;
        case PrefixType::STUDDED:
            ac_bonus += 1;
            value_modifier += 40;
            break;
        case PrefixType::ELVEN:
            ac_bonus += 2;
            dexterity_bonus += 1;
            value_modifier += 300;
            break;
        case PrefixType::DWARVEN:
            ac_bonus += 3;
            strength_bonus += 1;
            value_modifier += 400;
            break;
        case PrefixType::MAGICAL:
            ac_bonus += 1;
            is_magical = true;
            value_modifier += 250;
            break;
        case PrefixType::CURSED:
            is_cursed = true;
            value_modifier = 10; // Drastically reduced value
            break;
        case PrefixType::ANCIENT:
            value_modifier += 500;
            break;
        case PrefixType::RUSTED:
            damage_bonus -= 1;
            ac_bonus -= 1;
            value_modifier = 30;
            break;
        case PrefixType::CRACKED:
            value_modifier = 50;
            break;
        default:
            break;
    }
    
    // Apply suffix effects
    switch (suffix)
    {
        case SuffixType::OF_SLAYING:
            damage_bonus += 3;
            value_modifier += 300;
            break;
        case SuffixType::OF_ACCURACY:
            to_hit_bonus += 2;
            value_modifier += 200;
            break;
        case SuffixType::OF_PROTECTION:
            ac_bonus += 2;
            value_modifier += 250;
            break;
        case SuffixType::OF_POWER:
            strength_bonus += 1;
            dexterity_bonus += 1;
            intelligence_bonus += 1;
            value_modifier += 400;
            break;
        case SuffixType::OF_FIRE_RESISTANCE:
            fire_resistance = 50;
            value_modifier += 150;
            break;
        case SuffixType::OF_COLD_RESISTANCE:
            cold_resistance = 50;
            value_modifier += 150;
            break;
        case SuffixType::OF_LIGHTNING_RESISTANCE:
            lightning_resistance = 50;
            value_modifier += 150;
            break;
        case SuffixType::OF_POISON_RESISTANCE:
            poison_resistance = 50;
            value_modifier += 150;
            break;
        case SuffixType::OF_SPEED:
            speed_bonus += 2;
            value_modifier += 200;
            break;
        case SuffixType::OF_STEALTH:
            stealth_bonus += 3;
            value_modifier += 175;
            break;
        case SuffixType::OF_MAGIC:
            mana_bonus += 20;
            intelligence_bonus += 1;
            value_modifier += 225;
            break;
        case SuffixType::OF_HEALTH:
            hp_bonus += 15;
            value_modifier += 200;
            break;
        case SuffixType::OF_THE_EAGLE:
            dexterity_bonus += 2;
            value_modifier += 150;
            break;
        case SuffixType::OF_THE_BEAR:
            strength_bonus += 2;
            value_modifier += 150;
            break;
        case SuffixType::OF_THE_OWL:
            intelligence_bonus += 2;
            value_modifier += 150;
            break;
        case SuffixType::OF_WEAKNESS:
            strength_bonus -= 1;
            is_cursed = true;
            value_modifier = 20;
            break;
        case SuffixType::OF_SLOWNESS:
            speed_bonus -= 2;
            is_cursed = true;
            value_modifier = 15;
            break;
        case SuffixType::OF_BRITTLENESS:
            is_cursed = true;
            value_modifier = 25;
            break;
        default:
            break;
    }
}

// Random generation methods
ItemEnhancement ItemEnhancement::generate_random_enhancement(bool allow_magical)
{
    ItemEnhancement enhancement;
    
    // 30% chance for prefix, 25% chance for suffix, 5% chance for both
    int roll = rand() % 100;
    
    if (roll < 30) // Prefix only
    {
        enhancement.prefix = get_random_universal_prefix();
    }
    else if (roll < 55) // Suffix only
    {
        enhancement.suffix = get_random_special_suffix();
    }
    else if (roll < 60) // Both prefix and suffix
    {
        enhancement.prefix = get_random_universal_prefix();
        enhancement.suffix = get_random_special_suffix();
    }
    // 40% chance for no enhancement
    
    if (!allow_magical)
    {
        // Remove magical prefixes if not allowed
        if (enhancement.prefix == PrefixType::FLAMING || 
            enhancement.prefix == PrefixType::FROST || 
            enhancement.prefix == PrefixType::SHOCK ||
            enhancement.prefix == PrefixType::MAGICAL)
        {
            enhancement.prefix = PrefixType::NONE;
        }
    }
    
    enhancement.apply_enhancement_effects();
    return enhancement;
}

ItemEnhancement ItemEnhancement::generate_weapon_enhancement()
{
    ItemEnhancement enhancement;
    
    int roll = rand() % 100;
    if (roll < 40)
    {
        enhancement.prefix = get_random_weapon_prefix();
    }
    if (roll >= 20 && roll < 60)
    {
        enhancement.suffix = get_random_combat_suffix();
    }
    
    enhancement.apply_enhancement_effects();
    return enhancement;
}

ItemEnhancement ItemEnhancement::generate_armor_enhancement()
{
    ItemEnhancement enhancement;
    
    int roll = rand() % 100;
    if (roll < 35)
    {
        enhancement.prefix = get_random_armor_prefix();
    }
    if (roll >= 25 && roll < 55)
    {
        enhancement.suffix = get_random_resistance_suffix();
    }
    
    enhancement.apply_enhancement_effects();
    return enhancement;
}

ItemEnhancement ItemEnhancement::generate_by_rarity(int rarity_level)
{
    ItemEnhancement enhancement;
    
    // Higher rarity = more likely to have enhancements
    int prefix_chance = rarity_level * 15; // 15%, 30%, 45%, 60%, 75%
    int suffix_chance = rarity_level * 12; // 12%, 24%, 36%, 48%, 60%
    
    if (rand() % 100 < prefix_chance)
    {
        enhancement.prefix = get_random_universal_prefix();
    }
    
    if (rand() % 100 < suffix_chance)
    {
        enhancement.suffix = get_random_special_suffix();
    }
    
    enhancement.apply_enhancement_effects();
    return enhancement;
}

// Private helper methods
PrefixType ItemEnhancement::get_random_weapon_prefix()
{
    static const PrefixType weapon_prefixes[] = {
        PrefixType::SHARP, PrefixType::KEEN, PrefixType::MASTERWORK, 
        PrefixType::BLESSED, PrefixType::FLAMING, PrefixType::FROST, 
        PrefixType::SHOCK, PrefixType::ANCIENT, PrefixType::CURSED
    };
    
    return weapon_prefixes[rand() % (sizeof(weapon_prefixes) / sizeof(weapon_prefixes[0]))];
}

PrefixType ItemEnhancement::get_random_armor_prefix()
{
    static const PrefixType armor_prefixes[] = {
        PrefixType::REINFORCED, PrefixType::STUDDED, PrefixType::ELVEN, 
        PrefixType::DWARVEN, PrefixType::MAGICAL, PrefixType::ANCIENT, 
        PrefixType::CURSED, PrefixType::RUSTED
    };
    
    return armor_prefixes[rand() % (sizeof(armor_prefixes) / sizeof(armor_prefixes[0]))];
}

PrefixType ItemEnhancement::get_random_universal_prefix()
{
    static const PrefixType universal_prefixes[] = {
        PrefixType::BLESSED, PrefixType::CURSED, PrefixType::ANCIENT, 
        PrefixType::MAGICAL, PrefixType::RUSTED, PrefixType::CRACKED
    };
    
    return universal_prefixes[rand() % (sizeof(universal_prefixes) / sizeof(universal_prefixes[0]))];
}

SuffixType ItemEnhancement::get_random_combat_suffix()
{
    static const SuffixType combat_suffixes[] = {
        SuffixType::OF_SLAYING, SuffixType::OF_ACCURACY, SuffixType::OF_PROTECTION,
        SuffixType::OF_POWER, SuffixType::OF_THE_BEAR, SuffixType::OF_THE_EAGLE
    };
    
    return combat_suffixes[rand() % (sizeof(combat_suffixes) / sizeof(combat_suffixes[0]))];
}

SuffixType ItemEnhancement::get_random_resistance_suffix()
{
    static const SuffixType resistance_suffixes[] = {
        SuffixType::OF_FIRE_RESISTANCE, SuffixType::OF_COLD_RESISTANCE,
        SuffixType::OF_LIGHTNING_RESISTANCE, SuffixType::OF_POISON_RESISTANCE
    };
    
    return resistance_suffixes[rand() % (sizeof(resistance_suffixes) / sizeof(resistance_suffixes[0]))];
}

SuffixType ItemEnhancement::get_random_special_suffix()
{
    static const SuffixType special_suffixes[] = {
        SuffixType::OF_SPEED, SuffixType::OF_STEALTH, SuffixType::OF_MAGIC,
        SuffixType::OF_HEALTH, SuffixType::OF_THE_OWL, SuffixType::OF_WEAKNESS,
        SuffixType::OF_SLOWNESS, SuffixType::OF_BRITTLENESS
    };
    
    return special_suffixes[rand() % (sizeof(special_suffixes) / sizeof(special_suffixes[0]))];
}
