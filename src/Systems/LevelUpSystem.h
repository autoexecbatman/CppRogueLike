#pragma once

class Creature;
struct GameContext;

// Handles combat improvements on level up according to AD&D 2e rules.
// Each function encapsulates one domain rule: THAC0 progression, HP gain,
// class-specific milestones, and saving throw improvements.
namespace LevelUpSystem
{
    // Apply all level-up benefits to owner based on their class and newLevel.
    void apply_level_up_benefits(Creature& owner, int newLevel, GameContext* ctx);

    // AD&D 2e backstab table: x2 at level 1, +1 multiplier every 4 levels.
    // Public for display use in LevelUpUI.
    int calculate_backstab_multiplier(int level);
}
