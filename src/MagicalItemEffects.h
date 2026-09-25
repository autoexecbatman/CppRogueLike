#pragma once

#include <algorithm>
#include <array>

#include <format>
#include <stdexcept>
#include <string_view>

// Authentic AD&D 2e magical item effects
enum class MagicalEffect
{
	NONE,

	// Helm effects (Authentic AD&D 2e)
	BRILLIANCE, // armor of its own enchanted value, fire resistance, light (rare)
	TELEPORTATION, // Teleport at will
	TELEPATHY, // Read thoughts
	UNDERWATER_ACTION, // Breathe underwater

	// Gauntlet effects
	SWIMMING, // Swim, which in this game is crossing water

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

// Every MagicalEffect, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<MagicalEffect, 13> ALL_MAGICAL_EFFECT = {
	MagicalEffect::NONE,
	MagicalEffect::BRILLIANCE,
	MagicalEffect::TELEPORTATION,
	MagicalEffect::TELEPATHY,
	MagicalEffect::UNDERWATER_ACTION,
	MagicalEffect::SWIMMING,
	MagicalEffect::FREE_ACTION,
	MagicalEffect::REGENERATION,
	MagicalEffect::INVISIBILITY,
	MagicalEffect::FIRE_RESISTANCE,
	MagicalEffect::COLD_RESISTANCE,
	MagicalEffect::SPELL_STORING,
	MagicalEffect::PROTECTION,
};

// The next MagicalEffect in that order, wrapping at the end.
//
// Example:
//   next_magical_effect(MagicalEffect::NONE);   // -> MagicalEffect::BRILLIANCE
[[nodiscard]] inline MagicalEffect next_magical_effect(MagicalEffect value)
{
	const auto found = std::ranges::find(ALL_MAGICAL_EFFECT, value);
	if (found == ALL_MAGICAL_EFFECT.end() || found + 1 == ALL_MAGICAL_EFFECT.end())
	{
		return ALL_MAGICAL_EFFECT.front();
	}
	return *(found + 1);
}

// Helper for describing effects
namespace MagicalEffectUtils
{
const char* get_effect_description(MagicalEffect effect);
bool is_protection_effect(MagicalEffect effect);
int get_protection_bonus(MagicalEffect effect);
int get_ac_bonus(MagicalEffect effect, int bonus); // Returns AC bonus for any effect (protection rings, helms, etc.)
} // namespace MagicalEffectUtils

inline std::string_view encode_magical_effect(MagicalEffect magicalEffect)
{
	switch (magicalEffect)
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

	case MagicalEffect::SWIMMING:
	{
		return "swimming";
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

inline MagicalEffect parse_magical_effect(std::string_view name)
{
	if (name == "none")
	{
		return MagicalEffect::NONE;
	}
	if (name == "brilliance")
	{
		return MagicalEffect::BRILLIANCE;
	}
	if (name == "teleportation")
	{
		return MagicalEffect::TELEPORTATION;
	}
	if (name == "telepathy")
	{
		return MagicalEffect::TELEPATHY;
	}
	if (name == "swimming")
	{
		return MagicalEffect::SWIMMING;
	}
	if (name == "underwater_action")
	{
		return MagicalEffect::UNDERWATER_ACTION;
	}
	if (name == "free_action")
	{
		return MagicalEffect::FREE_ACTION;
	}
	if (name == "regeneration")
	{
		return MagicalEffect::REGENERATION;
	}
	if (name == "invisibility")
	{
		return MagicalEffect::INVISIBILITY;
	}
	if (name == "fire_resistance")
	{
		return MagicalEffect::FIRE_RESISTANCE;
	}
	if (name == "cold_resistance")
	{
		return MagicalEffect::COLD_RESISTANCE;
	}
	if (name == "spell_storing")
	{
		return MagicalEffect::SPELL_STORING;
	}
	if (name == "protection")
	{
		return MagicalEffect::PROTECTION;
	}

	throw std::runtime_error(std::format("unknown magical_effect '{}'", name));
}
