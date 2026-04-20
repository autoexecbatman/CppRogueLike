#pragma once

// Blessing status: Three-state enum per AD&D 2e model
enum class BlessingStatus
{
	UNCURSED, // Default, normal item
	BLESSED,  // Divine favor, bonuses
	CURSED    // Malevolent, penalties or entrapment
};

// Item identification status: tracks what the player has learned
struct ItemIdentificationStatus
{
	// Identification flags: what has the player discovered?
	bool identified_type{ false };        // Do we know the base item type? (sword vs dagger)
	bool identified_enhancement{ false }; // Do we know the enhancement level/prefix/suffix?
	bool identified_buc{ false };         // Do we know the blessing status?

	// Reset identification to unknown
	void reset() noexcept
	{
		identified_type = false;
		identified_enhancement = false;
		identified_buc = false;
	}

	// Mark as fully identified
	void identify_all() noexcept
	{
		identified_type = true;
		identified_enhancement = true;
		identified_buc = true;
	}

	// Check if fully identified
	[[nodiscard]] bool is_fully_identified() const noexcept
	{
		return identified_type && identified_enhancement && identified_buc;
	}
};
