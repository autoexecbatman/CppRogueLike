// file: Encumbrance.cpp
#include "Encumbrance.h"

std::string_view encumbrance_band_name(EncumbranceBand band)
{
	switch (band)
	{
	case EncumbranceBand::UNENCUMBERED:
	{
		return "Unencumbered";
	}
	case EncumbranceBand::LIGHT:
	{
		return "Light";
	}
	case EncumbranceBand::MODERATE:
	{
		return "Moderate";
	}
	case EncumbranceBand::HEAVY:
	{
		return "Heavy";
	}
	case EncumbranceBand::SEVERE:
	{
		return "Severe";
	}
	case EncumbranceBand::OVERLOADED:
	{
		return "Overloaded";
	}
	}

	return "Unencumbered";
}

std::optional<EncumbranceBand> encumbrance_band(int carriedPounds, const StrengthAttributes& row)
{
	// Past the Max. Carried Weight the character cannot move at all. The table prints
	// that column on every row, so this answers at a Strength whose bands are blank.
	if (carriedPounds > row.maxCarried)
	{
		return EncumbranceBand::OVERLOADED;
	}

	// The five graded bands are columns the book leaves blank outside Strength 2 to
	// 18/00, so a load this row can carry has no name.
	if (!row.encumbrance.has_value())
	{
		return std::nullopt;
	}

	// Each boundary is the last pound its band holds.
	const EncumbranceBands& bands = *row.encumbrance;
	if (carriedPounds <= bands.unencumberedTo)
	{
		return EncumbranceBand::UNENCUMBERED;
	}
	if (carriedPounds <= bands.lightTo)
	{
		return EncumbranceBand::LIGHT;
	}
	if (carriedPounds <= bands.moderateTo)
	{
		return EncumbranceBand::MODERATE;
	}
	if (carriedPounds <= bands.heavyTo)
	{
		return EncumbranceBand::HEAVY;
	}
	return EncumbranceBand::SEVERE;
}

// end of file: Encumbrance.cpp
