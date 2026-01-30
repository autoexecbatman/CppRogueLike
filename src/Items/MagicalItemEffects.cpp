#include "MagicalItemEffects.h"

namespace MagicalEffectUtils
{
    const char* get_effect_description(MagicalEffect effect)
    {
        switch (effect)
        {
            case MagicalEffect::BRILLIANCE:
                return "Grants +4 AC";
            case MagicalEffect::TELEPORTATION:
                return "Teleport at will";
            case MagicalEffect::FREE_ACTION:
                return "Immune to paralysis, web, and hold spells";
            case MagicalEffect::REGENERATION:
                return "Regenerate 1 HP per turn";
            case MagicalEffect::INVISIBILITY:
                return "Turn invisible at will";
            case MagicalEffect::PROTECTION_PLUS_1:
                return "Protection +1 to AC and saves";
            case MagicalEffect::PROTECTION_PLUS_2:
                return "Protection +2 to AC and saves";
            case MagicalEffect::PROTECTION_PLUS_3:
                return "Protection +3 to AC and saves";
            default:
                return "No special effect";
        }
    }

    bool is_protection_effect(MagicalEffect effect)
    {
        return effect == MagicalEffect::PROTECTION_PLUS_1 ||
               effect == MagicalEffect::PROTECTION_PLUS_2 ||
               effect == MagicalEffect::PROTECTION_PLUS_3;
    }

    int get_protection_bonus(MagicalEffect effect)
    {
        switch (effect)
        {
            case MagicalEffect::PROTECTION_PLUS_1: return -1;
            case MagicalEffect::PROTECTION_PLUS_2: return -2;
            case MagicalEffect::PROTECTION_PLUS_3: return -3;
            default: return 0;
        }
    }

    int get_ac_bonus(MagicalEffect effect)
    {
        switch (effect)
        {
            case MagicalEffect::BRILLIANCE: return -4;
            case MagicalEffect::PROTECTION_PLUS_1: return -1;
            case MagicalEffect::PROTECTION_PLUS_2: return -2;
            case MagicalEffect::PROTECTION_PLUS_3: return -3;
            default: return 0;
        }
    }
}
