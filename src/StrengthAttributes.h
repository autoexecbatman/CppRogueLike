#pragma once

#include <string>

struct StrengthAttributes
{
	int Str{};
	int hitProb{};
	int dmgAdj{};
	int wgtAllow{};
	int maxPress{};
	// The most this Strength can carry and still move, in pounds: Table 47's Max. Carried
	// Weight, or Table 1's Max. Press on the rows Table 47 does not print - Strength 1 and
	// the giant scores. Distinct from maxPress, which is a lift overhead rather than a load.
	int maxCarried{};
	int openDoors{};
	double BB_LG{};
	std::string notes{};
	// An exceptional band of Strength 18 - 18/01-50 and on to 18/00, as 100 - covers
	// this percentile range; both are 0 on a row for a plain score.
	int exceptionalFrom{};
	int exceptionalTo{};
};