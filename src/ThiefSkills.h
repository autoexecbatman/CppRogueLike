#pragma once

// file: ThiefSkills.h
//
// The thief's eight percentile skills, as Player's Handbook Tables 26 to 29 give
// them (PDF pages 84 to 86): a base score per skill, then adjustments for race,
// for Dexterity and for the armour worn, then the discretionary points the player
// has spent - "all thieves at 1st level receive 60 discretionary percentage
// points... Each time the thief rises a level in experience, the player receives
// another 30 points to distribute."
//
// The base scores already assume a thief in leather, which is why Table 29's "No
// Armor" column is a bonus and leather is not a column at all.
//
// The tables are here and the screen is in MenuThiefSkills, so what a point may do
// can be tested without a window. The racial column is Player's, beside the racial
// ability modifiers, because the race lives on Player.
//
// Usage:
//
//   const ThiefArmor worn = ThiefArmor::LEATHER;
//   thief_skill_score(ThiefSkill::OPEN_LOCKS, 10, 17, worn, 0);   // -> 30
//   //                base 10, dwarf +10, Dexterity 17 +10, leather 0, nothing spent
//
//   const ThiefSkillGrant grant = thief_skill_grant_at_level(1);  // -> { 60, 30 }
//   ThiefSkillAllocation allocation{ alreadySpent, grant, racial, 17, worn };
//   allocation.can_assign(ThiefSkill::OPEN_LOCKS);   // -> true
//   allocation.assign(ThiefSkill::OPEN_LOCKS);       // one point leaves the pool
//   allocation.total_points();                       // what the character keeps

#include <array>
#include <optional>
#include <string_view>

// The eight, in the order Table 26 prints them.
enum class ThiefSkill
{
	PICK_POCKETS,
	OPEN_LOCKS,
	FIND_REMOVE_TRAPS,
	MOVE_SILENTLY,
	HIDE_IN_SHADOWS,
	DETECT_NOISE,
	CLIMB_WALLS,
	READ_LANGUAGES
};

inline constexpr std::array<ThiefSkill, 8> ALL_THIEF_SKILL{
	ThiefSkill::PICK_POCKETS,
	ThiefSkill::OPEN_LOCKS,
	ThiefSkill::FIND_REMOVE_TRAPS,
	ThiefSkill::MOVE_SILENTLY,
	ThiefSkill::HIDE_IN_SHADOWS,
	ThiefSkill::DETECT_NOISE,
	ThiefSkill::CLIMB_WALLS,
	ThiefSkill::READ_LANGUAGES
};

// How many the eight are, for anything holding one value per skill.
inline constexpr std::size_t THIEF_SKILL_COUNT = ALL_THIEF_SKILL.size();

// Where a skill sits in an eight-element array.
//
// Example:
//   thief_skill_index(ThiefSkill::OPEN_LOCKS);   // -> 1
[[nodiscard]] constexpr std::size_t thief_skill_index(ThiefSkill skill)
{
	return static_cast<std::size_t>(skill);
}

// Table 29's columns. Leather is a column of zeroes rather than an absence: the
// base scores are a thief's in leather, so wearing it adjusts nothing.
enum class ThiefArmor
{
	NONE,
	LEATHER,
	PADDED_OR_ELVEN_CHAIN,
	HIDE_OR_STUDDED_LEATHER,
	CHAIN_OR_RING_MAIL
};

// The name a screen prints.
//
// Example:
//   thief_skill_name(ThiefSkill::FIND_REMOVE_TRAPS);   // -> "Find/Rem Traps"
[[nodiscard]] std::string_view thief_skill_name(ThiefSkill skill);

// The next of the eight, wrapping at the end, for a cursor walking the list.
//
// Example:
//   next_thief_skill(ThiefSkill::PICK_POCKETS);     // -> ThiefSkill::OPEN_LOCKS
//   next_thief_skill(ThiefSkill::READ_LANGUAGES);   // -> ThiefSkill::PICK_POCKETS
[[nodiscard]] ThiefSkill next_thief_skill(ThiefSkill skill);

// The previous of the eight, wrapping at the start.
//
// Example:
//   previous_thief_skill(ThiefSkill::PICK_POCKETS);   // -> ThiefSkill::READ_LANGUAGES
[[nodiscard]] ThiefSkill previous_thief_skill(ThiefSkill skill);

// Which of Table 29's columns an armour is read in, or nullopt where the table
// prints no column - every suit heavier than chain mail, which is also armour the
// book does not let a thief wear at all. An unknown key reads the same way.
//
// Example:
//   thief_armor_column("studded_leather");   // -> ThiefArmor::HIDE_OR_STUDDED_LEATHER
//   thief_armor_column("plate_mail");        // -> nullopt, no column
[[nodiscard]] std::optional<ThiefArmor> thief_armor_column(std::string_view armorItemKey);

// The percentage no skill passes: "no skill can be raised above 95 percent,
// including all adjustments for Dexterity, race, and armor."
inline constexpr int THIEF_SKILL_MAXIMUM = 95;

// Table 26's base score, which a thief in leather starts from.
//
// Example:
//   thief_skill_base(ThiefSkill::CLIMB_WALLS);   // -> 60
[[nodiscard]] int thief_skill_base(ThiefSkill skill);

// Table 28's column for a Dexterity. Only five skills have one; the other three
// are unaffected by Dexterity and answer 0.
//
// The book prints rows 9 through 19, the range a character reaches without magic.
// A score outside it reads the nearest printed row, so gauntlets carrying a thief
// past 19 cannot make the skill worse than 19 did.
//
// Example:
//   thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 18);    // -> 15
//   thief_skill_dexterity_adjustment(ThiefSkill::DETECT_NOISE, 18);  // -> 0
[[nodiscard]] int thief_skill_dexterity_adjustment(ThiefSkill skill, int dexterity);

// Table 29's column for the armour worn.
//
// Example:
//   thief_skill_armor_adjustment(ThiefSkill::MOVE_SILENTLY, ThiefArmor::NONE);      // -> 10
//   thief_skill_armor_adjustment(ThiefSkill::MOVE_SILENTLY, ThiefArmor::LEATHER);   // -> 0
[[nodiscard]] int thief_skill_armor_adjustment(ThiefSkill skill, ThiefArmor armor);

// The percentage a thief rolls against: the base, the three adjustments and the
// points spent, held between 0 and 95. A score the adjustments push below zero
// reads 0, which is the book's "the character must spend points raising his skill
// percentage to at least 1% before he can use the skill".
//
// Example, a halfling thief with Dexterity 17 in leather, 20 points on the skill:
//   thief_skill_score(ThiefSkill::HIDE_IN_SHADOWS, 15, 17, ThiefArmor::LEATHER, 20);
//   // -> 42, being base 5, halfling +15, Dexterity +5, leather 0, points 20
// An elf with Dexterity 9 and nothing spent cannot open a lock at all:
//   thief_skill_score(ThiefSkill::OPEN_LOCKS, -5, 9, ThiefArmor::LEATHER, 0);   // -> 0
[[nodiscard]] int thief_skill_score(
	ThiefSkill skill,
	int racialAdjustment,
	int dexterity,
	ThiefArmor armor,
	int pointsSpent);

// One level's worth of discretionary points, and how many of them one skill may
// take.
struct ThiefSkillGrant
{
	int points{ 0 };
	int perSkillLimit{ 0 };
};

// What a thief is handed on reaching a level: 60 points at 1st with no more than
// 30 on one skill, then 30 a level with no more than 15 on one skill.
//
// Example:
//   thief_skill_grant_at_level(1);   // -> { 60, 30 }
//   thief_skill_grant_at_level(4);   // -> { 30, 15 }
[[nodiscard]] ThiefSkillGrant thief_skill_grant_at_level(int level);

class ThiefSkillAllocation
{
private:
	// What earlier levels already put on each skill. Shown, counted in the score,
	// and not returnable here: this grant is the only thing being spent.
	std::array<int, THIEF_SKILL_COUNT> alreadySpent{};

	// What this grant has put on each skill.
	std::array<int, THIEF_SKILL_COUNT> assigned{};

	// Table 27's column for the character's race, one entry per skill.
	std::array<int, THIEF_SKILL_COUNT> racialAdjustment{};

	int dexterity{ 0 };
	ThiefArmor armor{ ThiefArmor::NONE };

	// Points of this grant not yet placed.
	int remaining{ 0 };

	// How many of this grant one skill may take.
	int perSkillLimit{ 0 };

public:
	ThiefSkillAllocation(
		std::array<int, THIEF_SKILL_COUNT> pointsAlreadySpent,
		ThiefSkillGrant grant,
		std::array<int, THIEF_SKILL_COUNT> raceAdjustment,
		int characterDexterity,
		ThiefArmor wornArmor);

	// What this grant has put on that skill.
	[[nodiscard]] int assigned_to(ThiefSkill skill) const;

	// The percentage the character would roll, every adjustment included.
	[[nodiscard]] int score(ThiefSkill skill) const;

	// Points of this grant still to place.
	[[nodiscard]] int remaining_points() const noexcept { return remaining; }

	// Whether one more point may go on that skill: a point must be left, this
	// grant's per-skill limit must not be reached, and the score must still be
	// under 95, since a point that cannot raise it is a point thrown away.
	[[nodiscard]] bool can_assign(ThiefSkill skill) const;

	// Moves one point out of the pool and onto that skill. Asks can_assign first.
	void assign(ThiefSkill skill);

	// Whether this grant put a point on that skill that can come back.
	[[nodiscard]] bool can_take_back(ThiefSkill skill) const;

	// Returns one of this grant's points from that skill to the pool.
	void take_back(ThiefSkill skill);

	// What the character keeps: everything earlier levels spent plus this grant.
	[[nodiscard]] std::array<int, THIEF_SKILL_COUNT> total_points() const;
};

// end of file: ThiefSkills.h
