// file: Config/GameBalance.h
// Centralized game balance constants - Single source of truth for all magic numbers
#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

#pragma once

namespace GameBalance
{
    // ========================================================================
    // SCROLLS & CONSUMABLES
    // ========================================================================
    namespace Scrolls
    {
        // Lightning Bolt Scroll
        inline constexpr int LIGHTNING_BOLT_RANGE = 5;
        inline constexpr int LIGHTNING_BOLT_DAMAGE = 20;
        inline constexpr int LIGHTNING_BOLT_VALUE = 150;  // Gold value

        // Fireball Scroll
        inline constexpr int FIREBALL_RANGE = 3;
        inline constexpr int FIREBALL_DAMAGE = 12;
        inline constexpr int FIREBALL_VALUE = 100;  // Gold value

        // Confusion Scroll
        inline constexpr int CONFUSION_RANGE = 10;
        inline constexpr int CONFUSION_TURNS = 8;
        inline constexpr int CONFUSION_VALUE = 120;  // Gold value

        // Teleportation Scroll
        inline constexpr int TELEPORTATION_VALUE = 200;  // Gold value
    }

    // ========================================================================
    // LEVELING & EXPERIENCE
    // ========================================================================
    namespace Leveling
    {
        // Ability Score Improvements (AD&D 2e)
        inline constexpr int ABILITY_SCORE_IMPROVEMENT_INTERVAL = 4;  // Every 4 levels

        // Fighter Progression
        namespace Fighter
        {
            inline constexpr int FIRST_EXTRA_ATTACK_LEVEL = 7;
            inline constexpr int SECOND_EXTRA_ATTACK_LEVEL = 13;
            inline constexpr int STRENGTH_IMPROVEMENT_INTERVAL = 3;  // Every 3 levels

            // Saving throw improvement levels
            inline constexpr int SAVING_THROW_LEVELS[] = {3, 6, 9, 12, 15};
        }

        // Rogue Progression
        namespace Rogue
        {
            // Backstab multiplier thresholds
            inline constexpr int BACKSTAB_MULTIPLIER_LEVEL_5 = 5;   // x3 damage
            inline constexpr int BACKSTAB_MULTIPLIER_LEVEL_9 = 9;   // x4 damage
            inline constexpr int BACKSTAB_MULTIPLIER_LEVEL_13 = 13; // x5 damage
            inline constexpr int BACKSTAB_BASE_MULTIPLIER = 2;      // x2 damage at level 1

            inline constexpr int STEALTH_IMPROVEMENT_INTERVAL = 2;  // Every 2 levels
            inline constexpr int SAVING_THROW_IMPROVEMENT_INTERVAL = 4;  // Every 4 levels
        }

        // Cleric Progression
        namespace Cleric
        {
            // Turn undead improvement levels
            inline constexpr int TURN_UNDEAD_IMPROVEMENT_LEVELS[] = {3, 5, 7, 9};
            inline constexpr int SPELL_ACCESS_MINIMUM_LEVEL = 2;
            inline constexpr int SAVING_THROW_IMPROVEMENT_INTERVAL = 3;  // Every 3 levels
        }

        // Wizard Progression
        namespace Wizard
        {
            inline constexpr int MAX_SPELL_LEVEL = 9;
            inline constexpr int SAVING_THROW_IMPROVEMENT_INTERVAL = 5;  // Every 5 levels
        }
    }

    // ========================================================================
    // COMBAT (AD&D 2e Mechanics)
    // ========================================================================
    namespace Combat
    {
        // Dice roll constants
        inline constexpr int D20_MAX = 20;  // For attack rolls, saving throws
        inline constexpr int CRITICAL_HIT = 20;
        inline constexpr int CRITICAL_MISS = 1;

        // Damage dice ranges (for display/validation)
        inline constexpr int D4_MIN = 1;
        inline constexpr int D4_MAX = 4;
        inline constexpr int D6_MIN = 1;
        inline constexpr int D6_MAX = 6;
        inline constexpr int D8_MIN = 1;
        inline constexpr int D8_MAX = 8;
        inline constexpr int D10_MIN = 1;
        inline constexpr int D10_MAX = 10;
        inline constexpr int D12_MIN = 1;
        inline constexpr int D12_MAX = 12;
        inline constexpr int D20_MIN = 1;

        // Unarmed combat
        inline constexpr int UNARMED_MIN_DAMAGE = 1;
        inline constexpr int UNARMED_MAX_DAMAGE = 2;  // 1d2
    }

    // ========================================================================
    // FIELD OF VIEW & RENDERING
    // ========================================================================
    namespace Vision
    {
        inline constexpr int FOV_RADIUS = 4;  // Tiles visible around player
    }

    // ========================================================================
    // ITEM ENHANCEMENTS
    // ========================================================================
    namespace Enhancements
    {
        inline constexpr int SPEED_BONUS_LIGHT_ARMOR = 2;   // +2 speed for light armor enchantment
        inline constexpr int SPEED_PENALTY_HEAVY_ARMOR = 2; // -2 speed for heavy armor curse
    }

    // ========================================================================
    // GUI & HEALTH DISPLAY
    // ========================================================================
    namespace GUI
    {
        // Health bar color thresholds (percentages)
        inline constexpr int HP_CRITICAL_THRESHOLD = 25;    // Red below 25%
        inline constexpr int HP_LOW_THRESHOLD = 40;         // Orange 26-40%
        inline constexpr int HP_MEDIUM_THRESHOLD = 60;      // Yellow 41-60%
        inline constexpr int HP_GOOD_THRESHOLD = 75;        // Light orange 61-75%
        // Above 75% = Green
    }
}

#endif // GAME_BALANCE_H
// end of file: Config/GameBalance.h
