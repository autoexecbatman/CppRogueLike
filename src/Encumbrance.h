#pragma once

// file: Encumbrance.h
//
// How burdened a character is by what it carries, as Player's Handbook Table 47
// names it (PDF page 159). The table prints five bands per Strength row -
// Unencumbered, Light, Moderate, Heavy, Severe - and a Max. Carried Weight past
// which the character cannot move at all.
//
// The bands are data rather than arithmetic because the book's are not derivable:
// above its allowance Strength 10-11 splits 18/18/20 where Strength 18 splits
// 39/39/39, so no fraction of the maximum reproduces the table. They ride on the
// Strength row beside the maximum they lead up to.
//
// Usage:
//
//   const StrengthAttributes row = dataManager.strength_for(10, 0);
//   encumbrance_band(40, row);    // -> UNENCUMBERED, the top of that row's first band
//   encumbrance_band(41, row);    // -> LIGHT
//   encumbrance_band(111, row);   // -> OVERLOADED, past the row's 110
//
//   const StrengthAttributes giant = dataManager.strength_for(21, 0);
//   encumbrance_band(400, giant); // -> nullopt, Table 47 prints no row for 21
//
// Nothing here says what an encumbrance band costs. Table 48 turns the bands into
// reduced movement rates, and this game has no movement rate to reduce, so the band
// is a description of the load and nothing more.

#include <array>
#include <optional>
#include <string_view>

#include "StrengthAttributes.h"

enum class EncumbranceBand
{
	UNENCUMBERED,
	LIGHT,
	MODERATE,
	HEAVY,
	SEVERE,
	OVERLOADED
};

inline constexpr std::array<EncumbranceBand, 6> ALL_ENCUMBRANCE_BAND{
	EncumbranceBand::UNENCUMBERED,
	EncumbranceBand::LIGHT,
	EncumbranceBand::MODERATE,
	EncumbranceBand::HEAVY,
	EncumbranceBand::SEVERE,
	EncumbranceBand::OVERLOADED
};

// The name Table 47 prints, or, past the table's last column, what carrying more
// than the maximum is.
//
// Example:
//   encumbrance_band_name(EncumbranceBand::SEVERE);   // -> "Severe"
[[nodiscard]] std::string_view encumbrance_band_name(EncumbranceBand band);

// The band this load falls in on this Strength row. A row Table 47 does not print -
// Strength 1, and 19 and up, where the book gives no bands at all - answers nullopt
// rather than a band the game made up.
//
// Example, the Strength 10-11 row:
//   encumbrance_band(0, row);     // -> UNENCUMBERED
//   encumbrance_band(97, row);    // -> SEVERE
//   encumbrance_band(110, row);   // -> SEVERE, the last pound it can carry
//   encumbrance_band(111, row);   // -> OVERLOADED
[[nodiscard]] std::optional<EncumbranceBand> encumbrance_band(int carriedPounds, const StrengthAttributes& row);

// end of file: Encumbrance.h
