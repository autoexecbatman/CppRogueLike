#pragma once

struct ConstitutionAttributes
{
	int Con{};
	int HPAdj{};
	// The least a rolled hit die counts as. Table 3's footnotes raise it from 20 up;
	// below that it is 1, which raises nothing.
	int hitDieMinimum{ 1 };
	int SystemShock{};
	int ResurrectionSurvival{};
	int PoisonSave{};
	int Regeneration{};
};