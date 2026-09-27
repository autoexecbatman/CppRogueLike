#pragma once

#include <vector>

struct WisdomAttributes
{
	int Wis{};
	int MagicalDefenseAdj{};
	// The spell levels this row adds to a priest's bonus spells, as Table 5 prints
	// them - row 19 carries a 1st and a 3rd. The column is cumulative, so what a
	// score is worth is every row up to it; that rule lives in SpellSystem.
	std::vector<int> bonusSpells{};
	int ChanceOfSpellFailure{};
	int SpellImmunity{};
};