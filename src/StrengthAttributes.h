#pragma once

#include <string>

struct StrengthAttributes
{
	int Str{};
	int hitProb{};
	int dmgAdj{};
	int wgtAllow{};
	int maxPress{};
	int openDoors{};
	double BB_LG{};
	std::string notes{};
	// An exceptional band of Strength 18 - 18/01-50 and on to 18/00, as 100 - covers
	// this percentile range; both are 0 on a row for a plain score.
	int exceptionalFrom{};
	int exceptionalTo{};
};