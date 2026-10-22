// file: Lifespan.cpp
#include <cassert>
#include <optional>
#include <string_view>

#include "AbilityAllocation.h"
#include "DiceExpr.h"
#include "Lifespan.h"
#include "PlayerRaceState.h"
#include "RandomDice.h"

namespace
{
// Whether the age has reached the band, so a band's change counts.
bool has_reached(int age, PlayerRaceState race, AgeCategory band)
{
	return age_category(age, race) >= band;
}
} // namespace

int Lifespan::natural_age() const noexcept
{
	return startingAge + yearsLived;
}

int Lifespan::physical_age() const noexcept
{
	return natural_age() + magicalYears;
}

std::optional<LifespanTable> lifespan_table(PlayerRaceState race)
{
	switch (race)
	{
	case PlayerRaceState::DWARF:
	{
		return LifespanTable{
			40,
			DiceExpr{ 5, 6, 0 },
			250,
			DiceExpr{ 2, 100, 0 },
			125,
			167,
			250,
		};
	}
	case PlayerRaceState::ELF:
	{
		return LifespanTable{
			100,
			DiceExpr{ 5, 6, 0 },
			350,
			DiceExpr{ 4, 100, 0 },
			175,
			233,
			350,
		};
	}
	case PlayerRaceState::GNOME:
	{
		return LifespanTable{
			60,
			DiceExpr{ 3, 12, 0 },
			200,
			DiceExpr{ 3, 100, 0 },
			100,
			133,
			200,
		};
	}
	case PlayerRaceState::HALFELF:
	{
		return LifespanTable{
			15,
			DiceExpr{ 1, 6, 0 },
			125,
			DiceExpr{ 3, 20, 0 },
			62,
			83,
			125,
		};
	}
	case PlayerRaceState::HALFLING:
	{
		return LifespanTable{
			20,
			DiceExpr{ 3, 4, 0 },
			100,
			DiceExpr{ 1, 100, 0 },
			50,
			67,
			100,
		};
	}
	case PlayerRaceState::HUMAN:
	{
		return LifespanTable{
			15,
			DiceExpr{ 1, 4, 0 },
			90,
			DiceExpr{ 2, 20, 0 },
			45,
			60,
			90,
		};
	}
	case PlayerRaceState::NONE:
	{
		return std::nullopt;
	}
	}

	assert(false && "lifespan_table: a PlayerRaceState with no row");
	return std::nullopt;
}

AgeCategory age_category(int age, PlayerRaceState race)
{
	const std::optional<LifespanTable> row = lifespan_table(race);
	if (!row)
	{
		return AgeCategory::YOUNG;
	}

	if (age >= row->venerableAge)
	{
		return AgeCategory::VENERABLE;
	}
	if (age >= row->oldAge)
	{
		return AgeCategory::OLD_AGE;
	}
	if (age >= row->middleAge)
	{
		return AgeCategory::MIDDLE_AGE;
	}
	return AgeCategory::YOUNG;
}

std::string_view age_category_name(AgeCategory category)
{
	switch (category)
	{
	case AgeCategory::YOUNG:
	{
		return "youth";
	}
	case AgeCategory::MIDDLE_AGE:
	{
		return "middle age";
	}
	case AgeCategory::OLD_AGE:
	{
		return "old age";
	}
	case AgeCategory::VENERABLE:
	{
		return "venerable age";
	}
	}

	assert(false && "age_category_name: an AgeCategory with no name");
	return "youth";
}

int ageing_change(const Lifespan& lifespan, Ability ability, int exceptionalStrength)
{
	// The body follows everything it has been through; the mind only what time has given it.
	const int physical = lifespan.physical_age();
	const int natural = lifespan.natural_age();
	const PlayerRaceState race = lifespan.race;
	auto body_reached = [physical, race](AgeCategory band)
	{
		return has_reached(physical, race, band);
	};
	auto mind_reached = [natural, race](AgeCategory band)
	{
		return has_reached(natural, race, band);
	};

	int change = 0;
	switch (ability)
	{
	case Ability::STRENGTH:
	{
		// Exceptional Strength is spent first: half instead of the point, then all of it and one point instead of two.
		const bool isExceptional = exceptionalStrength > 0;
		change -= body_reached(AgeCategory::MIDDLE_AGE) && !isExceptional ? 1 : 0;
		change -= body_reached(AgeCategory::OLD_AGE) ? (isExceptional ? 1 : 2) : 0;
		change -= body_reached(AgeCategory::VENERABLE) ? 1 : 0;
		break;
	}
	case Ability::DEXTERITY:
	{
		change -= body_reached(AgeCategory::OLD_AGE) ? 2 : 0;
		change -= body_reached(AgeCategory::VENERABLE) ? 1 : 0;
		break;
	}
	case Ability::CONSTITUTION:
	{
		change -= body_reached(AgeCategory::MIDDLE_AGE) ? 1 : 0;
		change -= body_reached(AgeCategory::OLD_AGE) ? 1 : 0;
		change -= body_reached(AgeCategory::VENERABLE) ? 1 : 0;
		break;
	}
	case Ability::INTELLIGENCE:
	{
		change += mind_reached(AgeCategory::MIDDLE_AGE) ? 1 : 0;
		change += mind_reached(AgeCategory::VENERABLE) ? 1 : 0;
		break;
	}
	case Ability::WISDOM:
	{
		change += mind_reached(AgeCategory::MIDDLE_AGE) ? 1 : 0;
		change += mind_reached(AgeCategory::OLD_AGE) ? 1 : 0;
		change += mind_reached(AgeCategory::VENERABLE) ? 1 : 0;
		break;
	}
	case Ability::CHARISMA:
	{
		break;
	}
	}
	return change;
}

int aged_exceptional_strength(const Lifespan& lifespan, int exceptionalStrength)
{
	const AgeCategory band = age_category(lifespan.physical_age(), lifespan.race);
	if (band >= AgeCategory::OLD_AGE)
	{
		return 0;
	}
	if (band == AgeCategory::MIDDLE_AGE)
	{
		return exceptionalStrength / 2;
	}
	return exceptionalStrength;
}

Lifespan roll_lifespan(PlayerRaceState race, RandomDice& dice)
{
	const std::optional<LifespanTable> row = lifespan_table(race);
	assert(row && "roll_lifespan: a character created with no race");

	// The starting age first, then the hidden maximum, in Table 11's column order.
	const int startingAge = row->startingBase + roll_dice(&dice, row->startingVariable);
	const int maximumAge = row->maximumBase + roll_dice(&dice, row->maximumVariable);
	return Lifespan{ race, startingAge, maximumAge, 0, 0 };
}

// end of file: Lifespan.cpp
