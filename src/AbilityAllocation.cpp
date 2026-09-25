// file: AbilityAllocation.cpp
#include <algorithm>
#include <cassert>
#include <numeric>

#include "RandomDice.h"
#include "AbilityAllocation.h"

std::string_view ability_name(Ability ability)
{
	switch (ability)
	{
	case Ability::STRENGTH:
	{
		return "Strength";
	}
	case Ability::DEXTERITY:
	{
		return "Dexterity";
	}
	case Ability::CONSTITUTION:
	{
		return "Constitution";
	}
	case Ability::INTELLIGENCE:
	{
		return "Intelligence";
	}
	case Ability::WISDOM:
	{
		return "Wisdom";
	}
	case Ability::CHARISMA:
	{
		return "Charisma";
	}
	}

	return "Strength";
}

Ability next_ability(Ability ability)
{
	const auto found = std::ranges::find(ALL_ABILITY, ability);
	if (found == ALL_ABILITY.end() || found + 1 == ALL_ABILITY.end())
	{
		return ALL_ABILITY.front();
	}
	return *(found + 1);
}

Ability previous_ability(Ability ability)
{
	const auto found = std::ranges::find(ALL_ABILITY, ability);
	if (found == ALL_ABILITY.end() || found == ALL_ABILITY.begin())
	{
		return ALL_ABILITY.back();
	}
	return *(found - 1);
}

std::optional<ClassMinimum> class_ability_minimum(CreatureClass creatureClass)
{
	// Player's Handbook Table 13. Every class here needs 9 in one ability; the
	// optional classes the table also lists want two or three and are not in the game.
	constexpr int TABLE_THIRTEEN_MINIMUM = 9;

	switch (creatureClass)
	{
	case CreatureClass::FIGHTER:
	{
		return ClassMinimum{ Ability::STRENGTH, TABLE_THIRTEEN_MINIMUM };
	}
	case CreatureClass::ROGUE:
	{
		return ClassMinimum{ Ability::DEXTERITY, TABLE_THIRTEEN_MINIMUM };
	}
	case CreatureClass::CLERIC:
	{
		return ClassMinimum{ Ability::WISDOM, TABLE_THIRTEEN_MINIMUM };
	}
	case CreatureClass::WIZARD:
	{
		return ClassMinimum{ Ability::INTELLIGENCE, TABLE_THIRTEEN_MINIMUM };
	}
	case CreatureClass::MONSTER:
	{
		return std::nullopt;
	}
	}

	return std::nullopt;
}

std::vector<int> roll_method_six_pool(RandomDice& dice)
{
	std::vector<int> pool;
	pool.reserve(static_cast<std::size_t>(METHOD_SIX_DICE));
	for (int die = 0; die < METHOD_SIX_DICE; ++die)
	{
		pool.push_back(dice.d6());
	}
	return pool;
}

AbilityAllocation::AbilityAllocation(
	std::vector<int> rolledPool,
	CreatureClass forClass,
	std::array<int, ABILITY_COUNT> raceModifier)
	: unplaced(std::move(rolledPool)), racialModifier(raceModifier), creatureClass(forClass)
{
}

int AbilityAllocation::allocated(Ability ability) const
{
	const std::vector<int>& dice = placed.at(ability_index(ability));
	return METHOD_SIX_STARTING_SCORE + std::accumulate(dice.begin(), dice.end(), 0);
}

int AbilityAllocation::score(Ability ability) const
{
	return allocated(ability) + racialModifier.at(ability_index(ability));
}

bool AbilityAllocation::can_spend(Ability ability, std::size_t dieIndex) const
{
	if (dieIndex >= unplaced.size())
	{
		return false;
	}

	// A die is spent whole, so one that overshoots cannot be spent here at all.
	return allocated(ability) + unplaced.at(dieIndex) <= ABILITY_MAXIMUM;
}

void AbilityAllocation::spend(Ability ability, std::size_t dieIndex)
{
	assert(can_spend(ability, dieIndex) && "AbilityAllocation::spend called with a die that does not fit");

	placed.at(ability_index(ability)).push_back(unplaced.at(dieIndex));
	unplaced.erase(unplaced.begin() + static_cast<std::ptrdiff_t>(dieIndex));
}

bool AbilityAllocation::has_placed(Ability ability) const
{
	return !placed.at(ability_index(ability)).empty();
}

void AbilityAllocation::take_back(Ability ability)
{
	assert(has_placed(ability) && "AbilityAllocation::take_back called on an ability holding no dice");

	std::vector<int>& dice = placed.at(ability_index(ability));
	unplaced.push_back(dice.back());
	dice.pop_back();
}

std::optional<ClassMinimum> AbilityAllocation::unmet_minimum() const
{
	const std::optional<ClassMinimum> required = class_ability_minimum(creatureClass);
	if (!required.has_value() || score(required->ability) >= required->score)
	{
		return std::nullopt;
	}
	return required;
}

std::array<int, ABILITY_COUNT> AbilityAllocation::allocated_scores() const
{
	std::array<int, ABILITY_COUNT> scores{};
	for (const Ability ability : ALL_ABILITY)
	{
		scores.at(ability_index(ability)) = allocated(ability);
	}
	return scores;
}

// end of file: AbilityAllocation.cpp
