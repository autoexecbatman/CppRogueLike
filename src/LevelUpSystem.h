#pragma once

class Creature;
struct GameContext;

#include "CreatureClass.h"

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

    // Which level a class stops rolling hit dice at, and what it gains per
    // level after that.
    struct HitPointProgression
    {
        int lastRolledLevel{ 0 };
        int flatGain{ 0 };
    };

    // AD&D 2e Player's Handbook Tables 14, 20, 23 and 25. A character rolls one
    // hit die per level up to lastRolledLevel; past it the book grants flatGain
    // hit points per level and, in its words, they "no longer gain additional
    // hit point bonuses for high Constitution scores".
    //
    // Monsters do not advance by these tables and take the warrior progression,
    // so every class has a defined answer.
    //
    // Example:
    //   hit_point_progression(CreatureClass::WIZARD).lastRolledLevel; // -> 10
    //   hit_point_progression(CreatureClass::WIZARD).flatGain;        // -> 1
    HitPointProgression hit_point_progression(CreatureClass creatureClass);

    // Whether a creature of this class adds its Constitution hit point
    // adjustment on this level. The book adds it to "each Hit Die rolled for the
    // character" (Player's Handbook, PDF page 32), so it holds on exactly the
    // levels hit_point_progression rolls, and never for a monster, whose hit
    // points are its hit dice.
    //
    // Example:
    //   takes_constitution_adjustment_at(CreatureClass::FIGHTER, 9);   // -> true
    //   takes_constitution_adjustment_at(CreatureClass::FIGHTER, 10);  // -> false, a flat 3 from here
    //   takes_constitution_adjustment_at(CreatureClass::MONSTER, 1);   // -> false
    bool takes_constitution_adjustment_at(CreatureClass creatureClass, int level);

    // How many of levels 1 through `level` took the adjustment - what a change
    // of Constitution score is multiplied by. The book's "multiplied by the
    // character's level (up to 10)" is read as these levels, since a warrior's
    // and a priest's adjustment ended at 9th.
    //
    // Example:
    //   levels_taking_constitution_adjustment(CreatureClass::WIZARD, 3);    // -> 3
    //   levels_taking_constitution_adjustment(CreatureClass::FIGHTER, 12);  // -> 9
    //   levels_taking_constitution_adjustment(CreatureClass::MONSTER, 13);  // -> 0
    int levels_taking_constitution_adjustment(CreatureClass creatureClass, int level);

    // The THAC0 a class attacks at on a given level. AD&D 2e Player's Handbook
    // attack tables: a warrior improves a point a level, a rogue a point every
    // two, a priest two points every three, a wizard a point every three.
    //
    // Monsters attack on the warrior table, and a level off either end of the
    // table answers 20, so every class and level has an answer.
    //
    // Example:
    //   thac0_for_level(CreatureClass::WIZARD, 3);   // -> 20
    //   thac0_for_level(CreatureClass::WIZARD, 4);   // -> 19
    int thac0_for_level(CreatureClass creatureClass, int level);

    // Whether reaching this level moves the class down its attack table.
    //
    // Read from the table rather than from the creature, whose THAC0 has already
    // been advanced by the time any display asks, so comparing against it says
    // yes at every level of every class.
    //
    // Example:
    //   thac0_improves_at(CreatureClass::FIGHTER, 3);  // -> true
    //   thac0_improves_at(CreatureClass::WIZARD, 3);   // -> false, 20 at both
    bool thac0_improves_at(CreatureClass creatureClass, int level);
}
