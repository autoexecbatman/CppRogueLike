// LevelUpSystem.h - Handles combat improvements on level up according to AD&D 2e rules

#ifndef LEVELUPSYSTEM_H
#define LEVELUPSYSTEM_H

#pragma once

class Creature;

class LevelUpSystem
{
public:
    // Main function to apply all level up benefits
    static void apply_level_up_benefits(Creature& owner, int newLevel);
    
    // Helper functions (public for display purposes)
    static int calculate_backstab_multiplier(int level);
    
private:
    // Core combat improvements
    static void apply_thac0_improvement(Creature& owner, int newLevel);
    static int apply_hit_point_gain(Creature& owner, int newLevel);
    static void apply_class_specific_improvements(Creature& owner, int newLevel);
    
    // AD&D 2e specific improvements
    static void apply_ability_score_improvement(Creature& owner, int newLevel);
    static void apply_saving_throw_improvements(Creature& owner, int newLevel);
    
    // Class-specific improvements
    static void apply_fighter_improvements(Creature& owner, int newLevel);
    static void apply_rogue_improvements(Creature& owner, int newLevel);
    static void apply_cleric_improvements(Creature& owner, int newLevel);
    static void apply_wizard_improvements(Creature& owner, int newLevel);
};

#endif // LEVELUPSYSTEM_H
