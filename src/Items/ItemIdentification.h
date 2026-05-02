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
	bool identifiedType{ false };        // Do we know the base item type? (sword vs dagger)
	bool identifiedEnhancement{ false }; // Do we know the enhancement level/prefix/suffix?
	bool identifiedBuc{ false };         // Do we know the blessing status?

	// Reset identification to unknown
	void reset() noexcept
	{
		identifiedType = false;
		identifiedEnhancement = false;
		identifiedBuc = false;
	}

	// Mark as fully identified
	void identify_all() noexcept
	{
		identifiedType = true;
		identifiedEnhancement = true;
		identifiedBuc = true;
	}

	// Check if fully identified
	[[nodiscard]] bool is_fully_identified() const noexcept
	{
		return identifiedType && identifiedEnhancement && identifiedBuc;
	}
};
