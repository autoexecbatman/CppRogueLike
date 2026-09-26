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
#include "ThiefSkills.h"

struct GameContext;
class Player;

// Opens the screen a thief spends this level's discretionary points on, reading
// the race, the Dexterity and the armour off the character rather than predicting
// them - Table 29's numbers depend on what is on its body, so this runs after the
// character is dressed. Does nothing for any other class.
//
// Example, a new rogue standing in the leather its kit gave it:
//   push_thief_skill_allocation(player, 1, ctx);   // 60 points, read in leather
// A fighter reaching 4th level:
//   push_thief_skill_allocation(player, 4, ctx);   // no screen, nothing to spend
void push_thief_skill_allocation(Player& character, int level, GameContext& ctx);

class MenuThiefSkills : public BaseMenu
{
private:
	ThiefSkillAllocation allocation;
	std::function<void(std::array<int, THIEF_SKILL_COUNT>, GameContext&)> onAccept;
	ThiefSkill cursor{ ThiefSkill::PICK_POCKETS };

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
		std::function<void(std::array<int, THIEF_SKILL_COUNT>, GameContext&)> acceptCallback);
	MenuThiefSkills(const MenuThiefSkills&) = delete;
	MenuThiefSkills& operator=(const MenuThiefSkills&) = delete;
	MenuThiefSkills(MenuThiefSkills&&) = delete;
	MenuThiefSkills& operator=(MenuThiefSkills&&) = delete;

	void menu(GameContext& ctx) override;
};

// end of file: MenuThiefSkills.h
