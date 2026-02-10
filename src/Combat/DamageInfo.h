#pragma once

#include <string>
#include <format>

#include "../Random/RandomDice.h"

// Damage type classification for resistance calculations
enum class DamageType
{
    PHYSICAL,    // Normal weapon damage
    FIRE,        // Fire damage
    COLD,        // Cold/ice damage
    LIGHTNING,   // Lightning/shock damage
    POISON,      // Poison damage
    ACID,        // Acid damage
    MAGIC,       // Pure magical damage
};

// - Robust damage value system replacing fragile roll strings
struct DamageInfo
{
    int minDamage;
    int maxDamage;
    std::string displayRoll; // For UI display only: "1d8", "1d6+1", etc.
    DamageType damageType;    // Type of damage for resistance calculations

    // Constructors
    DamageInfo() : minDamage(1), maxDamage(2), displayRoll("1d2"), damageType(DamageType::PHYSICAL) {}

    DamageInfo(int min, int max, const std::string& display, DamageType type = DamageType::PHYSICAL)
        : minDamage(min), maxDamage(max), displayRoll(display), damageType(type) {}
    
    // Core damage operations
    int roll_damage(RandomDice* dice) const
    {
        if (minDamage == maxDamage) return minDamage;
        return dice->roll(minDamage, maxDamage);
    }
    
    int get_average_damage() const { return (minDamage + maxDamage) / 2; }
    
    // Modification operations
    DamageInfo& add_bonus(int bonus)
    {
        minDamage += bonus;
        maxDamage += bonus;
        if (bonus > 0)
        {
            displayRoll += std::format("+{}", bonus);
        } else if (bonus < 0)
        {
            displayRoll += std::format("{}", bonus); // Already has minus sign
        }
        return *this;
    }

    // Create enhanced version with bonus (non-mutating)
    DamageInfo with_enhancement(int damage_bonus, int hit_bonus) const
    {
        DamageInfo enhanced = *this;
        enhanced.add_bonus(damage_bonus);
        return enhanced;
    }
    
    DamageInfo& multiply_damage(float multiplier)
    {
        minDamage = static_cast<int>(minDamage * multiplier);
        maxDamage = static_cast<int>(maxDamage * multiplier);
        // Don't modify display roll for multipliers - too complex
        return *this;
    }
    
    // Utility functions
    bool is_valid() const { return minDamage > 0 && maxDamage >= minDamage; }
    std::string get_damage_range() const
    {
        if (minDamage == maxDamage)
        {
            return std::format("{}", minDamage);
        }
        return std::format("{}-{}", minDamage, maxDamage);
    }
    
    // Comparison operators
    bool operator==(const DamageInfo& other) const 
    {
        return minDamage == other.minDamage && maxDamage == other.maxDamage;
    }
    
    bool operator!=(const DamageInfo& other) const { return !(*this == other); }
};

// Common damage values for easy reference
namespace DamageValues
{
    inline DamageInfo Unarmed() { return {1, 2, "1d2"}; }
    inline DamageInfo Dagger() { return {1, 4, "1d4"}; }
    inline DamageInfo ShortSword() { return {1, 6, "1d6"}; }
    inline DamageInfo LongSword() { return {1, 8, "1d8"}; }
    inline DamageInfo GreatSword() { return {1, 10, "1d10"}; }
    inline DamageInfo BattleAxe() { return {1, 8, "1d8"}; }
    inline DamageInfo WarHammer() { return {2, 5, "1d4+1"}; }
    inline DamageInfo Staff() { return {1, 6, "1d6"}; }
    inline DamageInfo LongBow() { return {1, 6, "1d6"}; }
}
