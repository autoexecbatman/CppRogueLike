#pragma once

#include <vector>

struct ConstitutionAttributes
{
	int Con{};
	int HPAdj{};
	int SystemShock{};
	int ResurrectionSurvival{};
	int PoisonSave{};
	int Regeneration{};
};

std::vector<ConstitutionAttributes> loadConstitutionAttributes();