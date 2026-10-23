#pragma once

// file: MenuThiefSkills.h
//
// The screen a thief spends discretionary percentage points on, at creation and
// again at every level: "all thieves at 1st level receive 60 discretionary
// percentage points that they can add to their base scores... Each time the thief
// rises a level in experience, the player receives another 30 points to distribute"
// (Player's Handbook, PDF page 84).
//
// The rules are in ThiefSkills and the drawing is here, so what a point may do
// needs no window to test.
//
// It is built with the allocation it is to spend and a callback saying where the
// result goes. Both callers reach it through push_thief_skill_allocation below,
// which is the one place that knows what a level hands a thief.
//
// Usage - the character's first sixty points, once it is dressed:
//
//   push_thief_skill_allocation(player, 1, ctx);
//
// Keys: up and down choose a skill, right and left move one point onto it and off
// it, space puts as many as will fit, and enter accepts.

#include <array>
#include <functional>
#include <string>

#include "BaseMenu.h"
#include "InputSystem.h"
#include "ThiefSkills.h"

struct GameContext;
class Player;

// The numbers this level's screen would be given, read off the character as it
// stands at the moment of the call: what earlier levels spent, what this level
// grants, Table 27's column for the race, the Dexterity it is carrying and the
// armour on its body. Armour Table 29 prints no column for is judged unarmoured,
// so the points are not stranded.
//
// It reads and pushes nothing, which is what lets the numbers be checked without a
// screen. Reading live state is also what makes the call order matter: run before
// the character is dressed it answers for a naked one.
//
// Example, the dwarf rogue with Dexterity 17 that ThiefSkillAllocationTest builds:
//   thief_skill_allocation_for(rogue, 1).score(ThiefSkill::MOVE_SILENTLY);  // -> 25
// The same character once its kit has put leather on it:
//   thief_skill_allocation_for(rogue, 1).score(ThiefSkill::MOVE_SILENTLY);  // -> 15
[[nodiscard]] ThiefSkillAllocation thief_skill_allocation_for(
	const Player& character,
	int level);

// Opens the screen a thief spends this level's discretionary points on, reading
// the race, the Dexterity and the armour off the character rather than predicting
// them - Table 29's numbers depend on what is on its body, so this runs after the
// character is dressed. Does nothing for any other class.
//
// Example, a new rogue standing in the leather its kit gave it:
//   push_thief_skill_allocation(player, 1, ctx);   // 60 points, read in leather
// A fighter reaching 4th level:
//   push_thief_skill_allocation(player, 4, ctx);   // no screen, nothing to spend
void push_thief_skill_allocation(
	Player& character,
	int level,
	GameContext& ctx);

// What the screen holds between keys: the skill the cursor is on, and the points as
// they are placed so far.
struct ThiefSkillScreenState
{
	ThiefSkill cursor{ ThiefSkill::PICK_POCKETS };
	ThiefSkillAllocation allocation;
};

// The screen as it stands after one key. Up and down move the cursor, wrapping at
// either end; right puts one point on the skill under it and left takes one back;
// space puts on as many as the grant, the per-skill limit and the 95 ceiling allow.
// A point the allocation refuses is not placed, and any other key, enter included,
// leaves the screen as it was: accepting is the menu's, since it hands the result on.
//
// Example, a 1st-level thief with the cursor on Pick Pockets and 60 points:
//   after_thief_skill_key(GameKey::RIGHT, screen); // 1 point on Pick Pockets, 59 left
//   after_thief_skill_key(GameKey::SPACE, screen); // 30 on Pick Pockets, the limit
//   after_thief_skill_key(GameKey::LEFT, screen); // nothing placed, so nothing changes
[[nodiscard]] ThiefSkillScreenState after_thief_skill_key(
	GameKey key,
	ThiefSkillScreenState screen);

class MenuThiefSkills : public BaseMenu
{
private:
	ThiefSkillScreenState screen;
	std::function<void(std::array<int, THIEF_SKILL_COUNT>, GameContext&)>
		onAccept;

	void draw_allocation_screen();

	// Pixels between the frame's left and right borders, which every line is
	// measured against so none draws past the box.
	[[nodiscard]] int interior_width() const;

	// One skill's line: its name, the percentage it stands at, what this grant has
	// put on it, and the marker when the cursor is on it.
	[[nodiscard]] std::string row_for(ThiefSkill skill) const;

	// What is left of the grant, and how much of it one skill may still take.
	[[nodiscard]] std::string pool_line() const;

	// What the screen is waiting for: points still to place, or that it is done.
	[[nodiscard]] std::string status_line() const;

public:
	MenuThiefSkills(
		GameContext& ctx,
		ThiefSkillAllocation startingAllocation,
		std::function<void(std::array<int, THIEF_SKILL_COUNT>, GameContext&)>
			acceptCallback);
	MenuThiefSkills(const MenuThiefSkills&) = delete;
	MenuThiefSkills& operator=(const MenuThiefSkills&) = delete;
	MenuThiefSkills(MenuThiefSkills&&) = delete;
	MenuThiefSkills& operator=(MenuThiefSkills&&) = delete;

	void menu(GameContext& ctx) override;
};

// end of file: MenuThiefSkills.h
