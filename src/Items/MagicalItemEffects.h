#pragma once

// Authentic AD&D 2e magical item effects
enum class MagicalEffect
{
    NONE,

    // Helm effects (Authentic AD&D 2e)
    BRILLIANCE,                 // +4 AC, fire resistance, light (rare)
    TELEPORTATION,              // Teleport at will

    // Ring effects
    FREE_ACTION,                // Immune to paralysis, web, hold
    REGENERATION,               // Heal 1 HP per turn
    INVISIBILITY,               // Turn invisible at will

    // Protection effects (special handling)
    PROTECTION_PLUS_1,          // +1 AC and saves (no stack with armor AC)
    PROTECTION_PLUS_2,          // +2 AC and saves (no stack with armor AC)
    PROTECTION_PLUS_3,          // +3 AC and saves (no stack with armor AC)

};

// Helper for describing effects
namespace MagicalEffectUtils
{
    const char* get_effect_description(MagicalEffect effect);
    bool is_protection_effect(MagicalEffect effect);
    int get_protection_bonus(MagicalEffect effect);
    int get_ac_bonus(MagicalEffect effect);  // Returns AC bonus for any effect (protection rings, helms, etc.)
}
