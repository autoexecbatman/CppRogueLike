#pragma once

// file: AbilityAllocation.h
//
// Method VI of the Player's Handbook's six ability score methods (PDF page 24), and
// the only one that allocates rather than rolls and keeps: "Each ability starts with
// a score of 8. Then roll seven dice. These dice can be added to your character's
// abilities as you wish. All the points on a die must be added to the same ability
// score... You can add as many dice as you want to any ability, but no ability score
// can exceed 18 points. If you cannot make an 18 by exact count on the dice, you
// cannot have an 18 score."
//
// The book offers it for the case this game is always in - "if you want to create a
// specific type of character" - because the class is chosen before any die is rolled.
//
// The rules are here and the drawing is in MenuAbilityScores, so what a die may do
// can be tested without a window.
//
// Usage:
//
//   const std::vector<int> pool = roll_method_six_pool(*ctx.dice);   // seven d6
//   AbilityAllocation allocation{ pool, CreatureClass::FIGHTER, racial_ability_modifiers(race) };
//
//   allocation.allocated(Ability::STRENGTH);        // -> 8, nothing spent yet
//   allocation.can_spend(Ability::STRENGTH, 0);     // -> true, the first die fits
//   allocation.spend(Ability::STRENGTH, 0);         // that die leaves the pool
//   allocation.score(Ability::STRENGTH);            // -> allocated plus the race
//   allocation.unmet_minimum();                     // -> Table 13's row, or nullopt
//   allocation.take_back(Ability::STRENGTH);        // the last die put there returns
//   allocation.allocated_scores();                  // the six, for the blueprint
//
// `allocated` is what the character is built with and `score` is what the player
// reads: a halfling's Strength shows one lower than it was allocated, and Table 13
// judges the lower number, because the race is paid before the character exists.

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "CreatureClass.h"

class RandomDice;

// The six, in the order every sheet and screen prints them.
enum class Ability
{
	STRENGTH,
	DEXTERITY,
	CONSTITUTION,
	INTELLIGENCE,
	WISDOM,
	CHARISMA
};

inline constexpr std::array<Ability, 6> ALL_ABILITY{
	Ability::STRENGTH,
	Ability::DEXTERITY,
	Ability::CONSTITUTION,
	Ability::INTELLIGENCE,
	Ability::WISDOM,
	Ability::CHARISMA
};

// How many the six are, for anything that holds one value per ability.
inline constexpr std::size_t ABILITY_COUNT = ALL_ABILITY.size();

// Where an ability sits in a six-element array.
//
// Example:
//   ability_index(Ability::CONSTITUTION);   // -> 2
[[nodiscard]] constexpr std::size_t ability_index(Ability ability)
{
	return static_cast<std::size_t>(ability);
}

// The name a screen prints.
//
// Example:
//   ability_name(Ability::INTELLIGENCE);   // -> "Intelligence"
[[nodiscard]] std::string_view ability_name(Ability ability);

// The next of the six, wrapping at the end, for a cursor walking the list.
//
// Example:
//   next_ability(Ability::STRENGTH);   // -> Ability::DEXTERITY
//   next_ability(Ability::CHARISMA);   // -> Ability::STRENGTH
[[nodiscard]] Ability next_ability(Ability ability);

// The previous of the six, wrapping at the start.
//
// Example:
//   previous_ability(Ability::STRENGTH);   // -> Ability::CHARISMA
[[nodiscard]] Ability previous_ability(Ability ability);

// One row of Player's Handbook Table 13, Class Ability Minimums (PDF page 53).
struct ClassMinimum
{
	Ability ability{ Ability::STRENGTH };
	int score{ 0 };
};

// What a class cannot be played without. Table 13 gives Fighter 9 Strength, Thief 9
// Dexterity, Cleric 9 Wisdom and Mage 9 Intelligence; the optional classes it also
// lists are not in this game, and a monster has no row.
//
// Example:
//   class_ability_minimum(CreatureClass::CLERIC)->score;   // -> 9
//   class_ability_minimum(CreatureClass::MONSTER);         // -> nullopt
[[nodiscard]] std::optional<ClassMinimum> class_ability_minimum(CreatureClass creatureClass);

// The score every ability starts Method VI at.
inline constexpr int METHOD_SIX_STARTING_SCORE = 8;

// How many dice the method rolls.
inline constexpr int METHOD_SIX_DICE = 7;

// The score no ability may pass.
inline constexpr int ABILITY_MAXIMUM = 18;

// The seven six-sided dice Method VI rolls, in the order they came up.
//
// Example, dice forced to 4, 4, 2, 1, 6, 3, 5:
//   roll_method_six_pool(dice);   // -> { 4, 4, 2, 1, 6, 3, 5 }
[[nodiscard]] std::vector<int> roll_method_six_pool(RandomDice& dice);

class AbilityAllocation
{
private:
	// Which dice went onto each ability, kept as rolled so one can be taken back
	// as itself. What an ability has taken is their sum rather than a second count.
	std::array<std::vector<int>, ABILITY_COUNT> placed{};

	// The dice not yet placed, in the order they were rolled.
	std::vector<int> unplaced{};

	// What the character's race will do to each score once it exists. Shown beside
	// the allocation and judged with it, never added to it.
	std::array<int, ABILITY_COUNT> racialModifier{};

	CreatureClass creatureClass{ CreatureClass::MONSTER };

public:
	AbilityAllocation(
		std::vector<int> rolledPool,
		CreatureClass forClass,
		std::array<int, ABILITY_COUNT> raceModifier);

	// What the character is built with: the starting 8 plus every die put here.
	[[nodiscard]] int allocated(Ability ability) const;

	// What the player reads: the allocation once the race has been paid.
	[[nodiscard]] int score(Ability ability) const;

	// The dice still to be placed.
	[[nodiscard]] const std::vector<int>& pool() const noexcept { return unplaced; }

	// Whether that die may go on that ability: the index must name a die still in
	// the pool, and a whole die that would carry the allocation past 18 cannot be
	// split, so it cannot be placed at all.
	[[nodiscard]] bool can_spend(Ability ability, std::size_t dieIndex) const;

	// Moves that die out of the pool and onto that ability. Asks can_spend first.
	void spend(Ability ability, std::size_t dieIndex);

	// Whether any die sits on that ability to be taken back.
	[[nodiscard]] bool has_placed(Ability ability) const;

	// Returns the last die placed on that ability to the end of the pool.
	void take_back(Ability ability);

	// The class row this allocation does not yet satisfy, judged on `score`, or
	// nullopt when the character may be accepted.
	[[nodiscard]] std::optional<ClassMinimum> unmet_minimum() const;

	// The six as the character is built with them, for the blueprint.
	[[nodiscard]] std::array<int, ABILITY_COUNT> allocated_scores() const;
};

// end of file: AbilityAllocation.h
