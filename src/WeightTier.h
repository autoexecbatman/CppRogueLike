#pragma once

// How burdened a creature is by what it carries, as a label for the inventory screen.
// How much it can carry at all is InventoryOperations::get_max_weight, which reads the
// Strength row's maxCarried.

enum class WeightTier
{
	LIGHT,
	MODERATE,
	HEAVY,
	OVERENCUMBERED
};

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
