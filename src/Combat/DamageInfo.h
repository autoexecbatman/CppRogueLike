// DamageInfo.h - Robust damage value system replacing fragile roll strings
#ifndef DAMAGE_INFO_H
#define DAMAGE_INFO_H

#pragma once

#include <string>
#include <random>

struct DamageInfo
{
    int minDamage;
    int maxDamage;
    std::string displayRoll; // For UI display only: "1d8", "1d6+1", etc.
    
    // Constructors
    DamageInfo() : minDamage(1), maxDamage(2), displayRoll("1d2") {}
    
    DamageInfo(int min, int max, const std::string& display)
        : minDamage(min), maxDamage(max), displayRoll(display) {}
    
    // Core damage operations
    int roll_damage() const 
    {
        if (minDamage == maxDamage) return minDamage;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(minDamage, maxDamage);
        return dis(gen);
    }
    
    int get_average_damage() const { return (minDamage + maxDamage) / 2; }
    
    // Modification operations
    DamageInfo& add_bonus(int bonus)
    {
        minDamage += bonus;
        maxDamage += bonus;
        if (bonus > 0) {
            displayRoll += "+" + std::to_string(bonus);
        } else if (bonus < 0) {
            displayRoll += std::to_string(bonus); // Already has minus sign
        }
        return *this;
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
        if (minDamage == maxDamage) {
            return std::to_string(minDamage);
        }
        return std::to_string(minDamage) + "-" + std::to_string(maxDamage);
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

#endif // DAMAGE_INFO_H