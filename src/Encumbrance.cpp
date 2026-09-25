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
	if (!row.encumbrance.has_value())
	{
		return std::nullopt;
	}

	// Past the Max. Carried Weight the character cannot move at all, which is the one
	// state the table does not name and the only one with a rule attached to it.
	if (carriedPounds > row.maxCarried)
	{
		return EncumbranceBand::OVERLOADED;
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
