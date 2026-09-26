// file: ThiefSkills.cpp
#include <algorithm>
#include <cassert>

#include "ThiefSkills.h"

namespace
{
// Table 26, Thieving Skill Base Scores, in ALL_THIEF_SKILL order.
constexpr std::array<int, THIEF_SKILL_COUNT> BASE_SCORE{ 15, 10, 5, 10, 5, 15, 60, 0 };

// The Dexterity rows Table 28 prints. A score outside them reads the nearest end.
constexpr int LOWEST_PRINTED_DEXTERITY = 9;
constexpr int HIGHEST_PRINTED_DEXTERITY = 19;
constexpr std::size_t DEXTERITY_ROWS = HIGHEST_PRINTED_DEXTERITY - LOWEST_PRINTED_DEXTERITY + 1;

// Table 28, Thieving Skill Dexterity Adjustments. Only the first five skills have
// a column; Detect Noise, Climb Walls and Read Languages are printed nowhere in it.
constexpr std::size_t DEXTERITY_COLUMNS = 5;
constexpr std::array<std::array<int, DEXTERITY_COLUMNS>, DEXTERITY_ROWS> DEXTERITY_ADJUSTMENT{ {
	// Pick Pockets, Open Locks, Find/Remove Traps, Move Silently, Hide in Shadows
	{ -15, -10, -10, -20, -10 }, //  9
	{ -10, -5, -10, -15, -5 }, // 10
	{ -5, 0, -5, -10, 0 }, // 11
	{ 0, 0, 0, -5, 0 }, // 12
	{ 0, 0, 0, 0, 0 }, // 13
	{ 0, 0, 0, 0, 0 }, // 14
	{ 0, 0, 0, 0, 0 }, // 15
	{ 0, 5, 0, 0, 0 }, // 16
	{ 5, 10, 0, 5, 5 }, // 17
	{ 10, 15, 5, 10, 10 }, // 18
	{ 15, 20, 10, 15, 15 } // 19
} };

// Table 29, Thieving Skill Armor Adjustments, one row per skill. Leather is the
// column of zeroes the base scores already assume, so it is not printed in the
// book; every other column is measured against it.
constexpr std::size_t ARMOR_COLUMNS = 5;
constexpr std::array<std::array<int, ARMOR_COLUMNS>, THIEF_SKILL_COUNT> ARMOR_ADJUSTMENT{ {
	// none, leather, padded or elven chain, hide or studded leather, chain or ring mail
	{ 5, 0, -20, -30, -25 }, // Pick Pockets
	{ 0, 0, -5, -10, -10 }, // Open Locks
	{ 0, 0, -5, -10, -10 }, // Find/Remove Traps
	{ 10, 0, -10, -20, -15 }, // Move Silently
	{ 5, 0, -10, -20, -15 }, // Hide in Shadows
	{ 0, 0, -5, -10, -5 }, // Detect Noise
	{ 10, 0, -20, -30, -25 }, // Climb Walls
	{ 0, 0, 0, 0, 0 } // Read Languages
} };

// One armour of the game's data read into one of Table 29's columns.
struct ArmorColumnEntry
{
	std::string_view itemKey{};
	ThiefArmor column{ ThiefArmor::LEATHER };
};

// Every armour the book gives a column to, by the key items.json holds it under.
// Elven chain has no record yet; anything heavier than chain mail has no column,
// which is also armour the book does not let a thief wear.
constexpr std::array<ArmorColumnEntry, 6> ARMOR_COLUMN_BY_KEY{ {
	{ "leather_armor", ThiefArmor::LEATHER },
	{ "padded_armor", ThiefArmor::PADDED_OR_ELVEN_CHAIN },
	{ "studded_leather", ThiefArmor::HIDE_OR_STUDDED_LEATHER },
	{ "hide_armor", ThiefArmor::HIDE_OR_STUDDED_LEATHER },
	{ "chain_mail", ThiefArmor::CHAIN_OR_RING_MAIL },
	{ "ring_mail", ThiefArmor::CHAIN_OR_RING_MAIL }
} };

// The five with a Dexterity column are the first five of the enum, which is what
// lets one index answer for both. Reordering the enum has to move the table too.
static_assert(ALL_THIEF_SKILL.at(0) == ThiefSkill::PICK_POCKETS);
static_assert(ALL_THIEF_SKILL.at(1) == ThiefSkill::OPEN_LOCKS);
static_assert(ALL_THIEF_SKILL.at(2) == ThiefSkill::FIND_REMOVE_TRAPS);
static_assert(ALL_THIEF_SKILL.at(3) == ThiefSkill::MOVE_SILENTLY);
static_assert(ALL_THIEF_SKILL.at(4) == ThiefSkill::HIDE_IN_SHADOWS);

// Which of Table 28's five columns a skill is in, or none of them.
[[nodiscard]] std::optional<std::size_t> dexterity_column_of(ThiefSkill skill)
{
	const std::size_t index = thief_skill_index(skill);
	if (index >= DEXTERITY_COLUMNS)
	{
		return std::nullopt;
	}
	return index;
}
} // namespace

std::string_view thief_skill_name(ThiefSkill skill)
{
	switch (skill)
	{
	case ThiefSkill::PICK_POCKETS:
	{
		return "Pick Pockets";
	}
	case ThiefSkill::OPEN_LOCKS:
	{
		return "Open Locks";
	}
	case ThiefSkill::FIND_REMOVE_TRAPS:
	{
		return "Find/Rem Traps";
	}
	case ThiefSkill::MOVE_SILENTLY:
	{
		return "Move Silently";
	}
	case ThiefSkill::HIDE_IN_SHADOWS:
	{
		return "Hide in Shadows";
	}
	case ThiefSkill::DETECT_NOISE:
	{
		return "Detect Noise";
	}
	case ThiefSkill::CLIMB_WALLS:
	{
		return "Climb Walls";
	}
	case ThiefSkill::READ_LANGUAGES:
	{
		return "Read Languages";
	}
	}

	return "Pick Pockets";
}

ThiefSkill next_thief_skill(ThiefSkill skill)
{
	const auto found = std::ranges::find(ALL_THIEF_SKILL, skill);
	if (found == ALL_THIEF_SKILL.end() || found + 1 == ALL_THIEF_SKILL.end())
	{
		return ALL_THIEF_SKILL.front();
	}
	return *(found + 1);
}

ThiefSkill previous_thief_skill(ThiefSkill skill)
{
	const auto found = std::ranges::find(ALL_THIEF_SKILL, skill);
	if (found == ALL_THIEF_SKILL.end() || found == ALL_THIEF_SKILL.begin())
	{
		return ALL_THIEF_SKILL.back();
	}
	return *(found - 1);
}

std::optional<ThiefArmor> thief_armor_column(std::string_view armorItemKey)
{
	// A key the table does not name is armour Table 29 prints no column for.
	const auto found = std::ranges::find(ARMOR_COLUMN_BY_KEY, armorItemKey, &ArmorColumnEntry::itemKey);
	if (found == ARMOR_COLUMN_BY_KEY.end())
	{
		return std::nullopt;
	}
	return found->column;
}

int thief_skill_base(ThiefSkill skill)
{
	return BASE_SCORE.at(thief_skill_index(skill));
}

int thief_skill_dexterity_adjustment(ThiefSkill skill, int dexterity)
{
	// Three of the eight have no Dexterity column at all.
	const std::optional<std::size_t> column = dexterity_column_of(skill);
	if (!column.has_value())
	{
		return 0;
	}

	// The book prints 9 through 19; magic reaches past both ends, and the row at
	// the end it passed is the last thing the table says.
	const int clamped = std::clamp(dexterity, LOWEST_PRINTED_DEXTERITY, HIGHEST_PRINTED_DEXTERITY);
	const std::size_t row = static_cast<std::size_t>(clamped - LOWEST_PRINTED_DEXTERITY);
	return DEXTERITY_ADJUSTMENT.at(row).at(column.value());
}

int thief_skill_armor_adjustment(ThiefSkill skill, ThiefArmor armor)
{
	return ARMOR_ADJUSTMENT.at(thief_skill_index(skill)).at(static_cast<std::size_t>(armor));
}

int thief_skill_score(
	ThiefSkill skill,
	int racialAdjustment,
	int dexterity,
	ThiefArmor armor,
	int pointsSpent)
{
	// The base, then the three tables, then what the player bought.
	const int total = thief_skill_base(skill)
		+ racialAdjustment
		+ thief_skill_dexterity_adjustment(skill, dexterity)
		+ thief_skill_armor_adjustment(skill, armor)
		+ pointsSpent;

	// A skill the adjustments push below zero is one the thief does not yet have.
	return std::clamp(total, 0, THIEF_SKILL_MAXIMUM);
}

ThiefSkillGrant thief_skill_grant_at_level(int level)
{
	// First level is the only one that hands out sixty.
	if (level <= 1)
	{
		return ThiefSkillGrant{ 60, 30 };
	}
	return ThiefSkillGrant{ 30, 15 };
}

ThiefSkillAllocation::ThiefSkillAllocation(
	std::array<int, THIEF_SKILL_COUNT> pointsAlreadySpent,
	ThiefSkillGrant grant,
	std::array<int, THIEF_SKILL_COUNT> raceAdjustment,
	int characterDexterity,
	ThiefArmor wornArmor)
	: alreadySpent{ pointsAlreadySpent }
	, racialAdjustment{ raceAdjustment }
	, dexterity{ characterDexterity }
	, armor{ wornArmor }
	, remaining{ grant.points }
	, perSkillLimit{ grant.perSkillLimit }
{
	assert(grant.points >= 0 && "ThiefSkillAllocation: a grant cannot be negative");
	assert(grant.perSkillLimit >= 0 && "ThiefSkillAllocation: a per-skill limit cannot be negative");
}

int ThiefSkillAllocation::assigned_to(ThiefSkill skill) const
{
	return assigned.at(thief_skill_index(skill));
}

int ThiefSkillAllocation::score(ThiefSkill skill) const
{
	const std::size_t index = thief_skill_index(skill);
	return thief_skill_score(
		skill,
		racialAdjustment.at(index),
		dexterity,
		armor,
		alreadySpent.at(index) + assigned.at(index));
}

bool ThiefSkillAllocation::can_assign(ThiefSkill skill) const
{
	// Nothing left in the pool.
	if (remaining <= 0)
	{
		return false;
	}

	// This grant has already put all it may on that skill.
	if (assigned_to(skill) >= perSkillLimit)
	{
		return false;
	}

	// A point that cannot raise the score is a point thrown away.
	return score(skill) < THIEF_SKILL_MAXIMUM;
}

void ThiefSkillAllocation::assign(ThiefSkill skill)
{
	assert(can_assign(skill) && "ThiefSkillAllocation::assign called where can_assign is false");

	++assigned.at(thief_skill_index(skill));
	--remaining;
}

bool ThiefSkillAllocation::can_take_back(ThiefSkill skill) const
{
	// Only this grant's points come back; earlier levels are spent.
	return assigned_to(skill) > 0;
}

void ThiefSkillAllocation::take_back(ThiefSkill skill)
{
	assert(can_take_back(skill) && "ThiefSkillAllocation::take_back called where can_take_back is false");

	--assigned.at(thief_skill_index(skill));
	++remaining;
}

std::array<int, THIEF_SKILL_COUNT> ThiefSkillAllocation::total_points() const
{
	std::array<int, THIEF_SKILL_COUNT> total{};
	for (std::size_t index = 0; index < THIEF_SKILL_COUNT; ++index)
	{
		total.at(index) = alreadySpent.at(index) + assigned.at(index);
	}
	return total;
}

// end of file: ThiefSkills.cpp
