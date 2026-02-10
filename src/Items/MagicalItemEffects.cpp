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
            case MagicalEffect::PROTECTION:
                return "Protection to AC and saves";
            default:
                return "No special effect";
        }
    }

    bool is_protection_effect(MagicalEffect effect)
    {
        return effect == MagicalEffect::PROTECTION;
    }

    int get_protection_bonus(MagicalEffect effect)
    {
        return effect == MagicalEffect::PROTECTION ? -1 : 0;
    }

    int get_ac_bonus(MagicalEffect effect, int bonus)
    {
        switch (effect)
        {
            case MagicalEffect::BRILLIANCE: return -4;
            case MagicalEffect::PROTECTION: return -bonus;
            default: return 0;
        }
    }
}
