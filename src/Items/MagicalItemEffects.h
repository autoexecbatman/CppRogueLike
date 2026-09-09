#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

// Authentic AD&D 2e magical item effects
enum class MagicalEffect
{
	NONE,

	// Helm effects (Authentic AD&D 2e)
	BRILLIANCE, // +4 AC, fire resistance, light (rare)
	TELEPORTATION, // Teleport at will
	TELEPATHY, // Read thoughts
	UNDERWATER_ACTION, // Breathe underwater

	// Ring effects
	FREE_ACTION, // Immune to paralysis, web, hold
	REGENERATION, // Heal 1 HP per turn
	INVISIBILITY, // Turn invisible at will
	FIRE_RESISTANCE, // Resist fire damage
	COLD_RESISTANCE, // Resist cold damage
	SPELL_STORING, // Store spells

	// Protection effects (bonus level stored in MagicalItemParams)
	PROTECTION, // +N AC and saves (no stack with armor AC)
};

// Helper for describing effects
namespace MagicalEffectUtils
{
const char* get_effect_description(MagicalEffect effect);
bool is_protection_effect(MagicalEffect effect);
int get_protection_bonus(MagicalEffect effect);
int get_ac_bonus(MagicalEffect effect, int bonus); // Returns AC bonus for any effect (protection rings, helms, etc.)
} // namespace MagicalEffectUtils

inline std::string_view encode_magical_effect(MagicalEffect e)
{
	switch (e)
	{

	case MagicalEffect::NONE:
	{
		return "none";
	}

	case MagicalEffect::BRILLIANCE:
	{
		return "brilliance";
	}

	case MagicalEffect::TELEPORTATION:
	{
		return "teleportation";
	}

	case MagicalEffect::TELEPATHY:
	{
		return "telepathy";
	}

	case MagicalEffect::UNDERWATER_ACTION:
	{
		return "underwater_action";
	}

	case MagicalEffect::FREE_ACTION:
	{
		return "free_action";
	}

	case MagicalEffect::REGENERATION:
	{
		return "regeneration";
	}

	case MagicalEffect::INVISIBILITY:
	{
		return "invisibility";
	}

	case MagicalEffect::FIRE_RESISTANCE:
	{
		return "fire_resistance";
	}

	case MagicalEffect::COLD_RESISTANCE:
	{
		return "cold_resistance";
	}

	case MagicalEffect::SPELL_STORING:
	{
		return "spell_storing";
	}

	case MagicalEffect::PROTECTION:
	{
		return "protection";
	}

	}
	
	return "none";
}

inline MagicalEffect parse_magical_effect(std::string_view s)
{
	if (s == "none")
	{
		return MagicalEffect::NONE;
	}
	if (s == "brilliance")
	{
		return MagicalEffect::BRILLIANCE;
	}
	if (s == "teleportation")
	{
		return MagicalEffect::TELEPORTATION;
	}
	if (s == "telepathy")
	{
		return MagicalEffect::TELEPATHY;
	}
	if (s == "underwater_action")
	{
		return MagicalEffect::UNDERWATER_ACTION;
	}
	if (s == "free_action")
	{
		return MagicalEffect::FREE_ACTION;
	}
	if (s == "regeneration")
	{
		return MagicalEffect::REGENERATION;
	}
	if (s == "invisibility")
	{
		return MagicalEffect::INVISIBILITY;
	}
	if (s == "fire_resistance")
	{
		return MagicalEffect::FIRE_RESISTANCE;
	}
	if (s == "cold_resistance")
	{
		return MagicalEffect::COLD_RESISTANCE;
	}
	if (s == "spell_storing")
	{
		return MagicalEffect::SPELL_STORING;
	}
	if (s == "protection")
	{
		return MagicalEffect::PROTECTION;
	}

	throw std::runtime_error(std::format("unknown magical_effect '{}'", s));
}
