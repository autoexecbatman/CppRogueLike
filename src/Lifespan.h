#pragma once

// file: Lifespan.h
//
// A character's race and age: how old they started, how long they may live, the years
// time has added and the years magic has added. The ability changes ageing brings are
// computed from these on every read and never written into a score.
//
// Two ages, because the book keeps them apart (PHB, PDF page 50):
// - the natural age, startingAge plus yearsLived, which is what time has done;
// - the physical age, the natural age plus magicalYears, which is what the body has been through.
// Intelligence and Wisdom follow the natural age, since their gains "are a function of the
// passage of game time". Strength, Dexterity and Constitution follow the physical age.
//
// Usage:
//
//   Lifespan human = roll_lifespan(PlayerRaceState::HUMAN, dice);   // 15+1d4, max 90+2d20
//   human.startingAge = 30;
//   human.magicalYears = 15;                                         // a body of 45
//   ageing_change(human, Ability::STRENGTH, 0);                      // -> -1, physical middle age
//   ageing_change(human, Ability::WISDOM, 0);                        // -> 0, the mind is 30
//   age_category(human.physical_age(), human.race);                  // -> AgeCategory::MIDDLE_AGE
//
// A race with no row, PlayerRaceState::NONE, never ages: every change is zero.

#include <optional>
#include <string_view>

#include "AbilityAllocation.h"
#include "DiceExpr.h"
#include "PlayerRaceState.h"

class RandomDice;

// The ageing bands of Table 12 (PHB, PDF page 51), youngest first.
enum class AgeCategory
{
	YOUNG,
	MIDDLE_AGE,
	OLD_AGE,
	VENERABLE
};

// One race's row of Table 11 and Table 12 (PHB, PDF page 51). The thresholds are the
// table's printed numbers: the text's one half and two thirds of the maximum do not
// reproduce them, since the half-elf's 62.5 is printed 62 and the halfling's 66.67 is 67.
struct LifespanTable
{
	// Table 11's base starting age, before the die.
	int startingBase{ 0 };
	// The die added to it.
	DiceExpr startingVariable{};
	// Table 11's base maximum age, which Table 12's thresholds are printed from.
	int maximumBase{ 0 };
	// The dice added to it for the hidden maximum.
	DiceExpr maximumVariable{};
	// Table 12's three printed ages: middle, old and venerable.
	int middleAge{ 0 };
	// Old age, the second band.
	int oldAge{ 0 };
	// Venerable, the base maximum.
	int venerableAge{ 0 };
};

// One character's race and the years behind them.
struct Lifespan
{
	// The character's race, which picks the Table 11 and 12 rows; NONE never ages.
	PlayerRaceState race{ PlayerRaceState::NONE };
	// The age the character began play at, rolled from Table 11.
	int startingAge{ 0 };
	// The age the character dies of old age at, rolled from Table 11 and never shown.
	int maximumAge{ 0 };
	// Whole years the game clock has passed since the character began.
	int yearsLived{ 0 };
	// Years magic has added, or taken away when negative.
	int magicalYears{ 0 };

	// What time has done: the starting age and the years lived.
	//
	// Example:
	//   Lifespan{ PlayerRaceState::HUMAN, 18, 100, 2, 5 }.natural_age();   // -> 20
	[[nodiscard]] int natural_age() const noexcept;

	// What the body has been through: the natural age and the magical years.
	//
	// Example:
	//   Lifespan{ PlayerRaceState::HUMAN, 18, 100, 2, 5 }.physical_age();   // -> 25
	[[nodiscard]] int physical_age() const noexcept;
};

// The race's row of Tables 11 and 12, or nothing for PlayerRaceState::NONE.
//
// Example:
//   lifespan_table(PlayerRaceState::HALFELF)->middleAge;   // -> 62
//   lifespan_table(PlayerRaceState::NONE);                 // -> std::nullopt
[[nodiscard]] std::optional<LifespanTable> lifespan_table(PlayerRaceState race);

// The band an age falls in for the race. A race with no row is always YOUNG.
//
// Example:
//   age_category(44, PlayerRaceState::HUMAN);   // -> AgeCategory::YOUNG
//   age_category(60, PlayerRaceState::HUMAN);   // -> AgeCategory::OLD_AGE
[[nodiscard]] AgeCategory age_category(int age, PlayerRaceState race);

// The band's name as a message says it.
//
// Example:
//   age_category_name(AgeCategory::OLD_AGE);   // -> "old age"
[[nodiscard]] std::string_view age_category_name(AgeCategory category);

// What ageing has done to one ability score, cumulative across the bands reached. A
// character with exceptional Strength loses half the percentile at middle age instead of
// the point, and all of it and one point at old age (PHB, PDF page 49).
//
// Example, a human of natural age 60 with no exceptional Strength:
//   ageing_change(lifespan, Ability::STRENGTH, 0);   // -> -3
//   ageing_change(lifespan, Ability::WISDOM, 0);     // -> 2
[[nodiscard]] int ageing_change(const Lifespan& lifespan, Ability ability, int exceptionalStrength);

// The exceptional Strength percentile ageing leaves: half at middle age, none from old age.
//
// Example, an 18/50 human:
//   aged_exceptional_strength(lifespanAt45, 50);   // -> 25
//   aged_exceptional_strength(lifespanAt60, 50);   // -> 0
[[nodiscard]] int aged_exceptional_strength(const Lifespan& lifespan, int exceptionalStrength);

// A new character's lifespan, rolled from the race's row of Table 11 with nothing lived.
// Refuses PlayerRaceState::NONE, which no character is created with.
//
// Example, the dice scripted to their least:
//   roll_lifespan(PlayerRaceState::HUMAN, dice);   // -> { HUMAN, 16, 92, 0, 0 }
[[nodiscard]] Lifespan roll_lifespan(PlayerRaceState race, RandomDice& dice);

// end of file: Lifespan.h
