#pragma once

#include <vector>

struct IntelligenceAttributes
{
	int Int{};
	int NumberOfLanguages{};
	int SpellLevel{};
	int ChanceToLearnSpell{};
	int MaxNumberOfSpells{};
	int IllusionImmunity{};
};

std::vector<IntelligenceAttributes> loadIntelligenceAttributes();