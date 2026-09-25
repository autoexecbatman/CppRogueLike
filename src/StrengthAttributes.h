#pragma once

#include <optional>
#include <string>

// Table 47's four band boundaries: the heaviest load each named band still holds.
// Severe runs from heavyTo + 1 up to the row's maxCarried, and anything past that is
// more than the character can carry at all.
struct EncumbranceBands
{
	int unencumberedTo{ 0 };
	int lightTo{ 0 };
	int moderateTo{ 0 };
	int heavyTo{ 0 };
};

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
	// Table 47's bands for this row. Absent on the scores the table does not print -
	// Strength 1, and 19 and up - where the book gives no bands at all rather than
	// bands this game would have to invent.
	std::optional<EncumbranceBands> encumbrance{};
};