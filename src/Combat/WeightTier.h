#pragma once

// D&D 2e encumbrance system based on STR ability score
// Tier progression: Light → Moderate → Heavy → Overencumbered

enum class WeightTier
{
	LIGHT,
	MODERATE,
	HEAVY,
	OVERENCUMBERED
};

// Calculate max carrying capacity in pounds based on STR
// D&D 2e formula: base 50 lbs + (STR - 10) * 5
inline int calculate_max_weight(int strength) noexcept
{
	return 50 + (strength - 10) * 5;
}

// Determine tier based on current weight vs max capacity
// Tiers: Light (0-1/3), Moderate (1/3-2/3), Heavy (2/3-max), Overencumbered (above max)
inline WeightTier get_weight_tier(int current_weight, int max_weight) noexcept
{
	if (current_weight > max_weight)
	{
		return WeightTier::OVERENCUMBERED;
	}

	const int light_threshold = max_weight / 3;
	const int moderate_threshold = (max_weight * 2) / 3;

	if (current_weight <= light_threshold)
	{
		return WeightTier::LIGHT;
	}
	if (current_weight <= moderate_threshold)
	{
		return WeightTier::MODERATE;
	}

	return WeightTier::HEAVY;
}
