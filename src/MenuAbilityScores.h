#pragma once

// file: MenuAbilityScores.h
//
// The character creation phase that spends Method VI's dice. It sits between the
// class menu and the name menu, because the class decides which Table 13 minimum
// the screen will not let the player leave unmet, and the race decides what each
// score will become once the character exists.
//
// The rules are in AbilityAllocation and the drawing is here, so nothing about what
// a die may do needs a window to test.
//
// Usage:
//
//   ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
//
// It reads the class, the race's modifiers and nothing else from the blueprint, and
// writes back the six allocated scores before pushing the name menu.
//
// Keys: up and down choose an ability, 1 to 7 put that die of the pool on it,
// backspace takes the last die back off it, and enter accepts once the class
// minimum is met.

#include <cstddef>
#include <optional>
#include <string>

#include "AbilityAllocation.h"
#include "BaseMenu.h"

struct GameContext;

// The line the screen prints for one ability: the marker when the cursor is on it,
// the name, what the dice put there, and what the race will make of it. The race is
// shown only where it does something, so a human's six lines all stay quiet.
//
// Example, a halfling who has put a 6 on Strength, cursor elsewhere:
//   ability_row_line(allocation, Ability::STRENGTH, Ability::DEXTERITY);
//                              // -> "  Strength      14 -1 = 13"
// A human's Constitution at the starting score, cursor on it:
//   ability_row_line(allocation, Ability::CONSTITUTION, Ability::CONSTITUTION);
//                              // -> "> Constitution   8"
[[nodiscard]] std::string ability_row_line(
	const AbilityAllocation& allocation,
	Ability ability,
	Ability cursor);

// The pool as the player picks from it, each die behind the digit that spends it.
// Says so in words once nothing is left, rather than printing an empty label.
//
// Example, the pool 4, 4, 2, 1, 6, 3, 5 as rolled, then once it is gone:
//   dice_pool_line(allocation);   // -> "Dice: 1)4 2)4 3)2 4)1 5)6 6)3 7)5"
//   dice_pool_line(spent);        // -> "Dice: none left"
[[nodiscard]] std::string dice_pool_line(const AbilityAllocation& allocation);

// What the screen is waiting for: the Table 13 minimum still unmet, or that the
// character may be accepted. Judged on the score the race will leave, not on the
// dice alone.
//
// Example, a halfling fighter who has spent nothing, then one who has met the row:
//   allocation_status_line(fresh);   // -> "Needs Strength 9"
//   allocation_status_line(ready);   // -> "Ready."
[[nodiscard]] std::string allocation_status_line(const AbilityAllocation& allocation);

// Which die of the pool a typed character names, counting from '1'. Method VI rolls
// METHOD_SIX_DICE of them, so a digit past the last and anything that is not one of
// those digits name none - including the zero the input system gives for no key.
//
// Example:
//   die_index_for_character('1');   // -> 0
//   die_index_for_character('7');   // -> 6
//   die_index_for_character('8');   // -> nullopt
//   die_index_for_character(0);     // -> nullopt
[[nodiscard]] std::optional<std::size_t> die_index_for_character(int charInput);

class MenuAbilityScores : public BaseMenu
{
private:
	AbilityAllocation allocation;
	Ability cursor{ Ability::STRENGTH };

	void draw_allocation_screen();

	// Pixels between the frame's left and right borders, which every line is
	// measured against so none draws past the box.
	[[nodiscard]] int interior_width() const;

public:
	MenuAbilityScores(GameContext& ctx);
	MenuAbilityScores(const MenuAbilityScores&) = delete;
	MenuAbilityScores& operator=(const MenuAbilityScores&) = delete;
	MenuAbilityScores(MenuAbilityScores&&) = delete;
	MenuAbilityScores& operator=(MenuAbilityScores&&) = delete;

	void menu(GameContext& ctx) override;
};

// end of file: MenuAbilityScores.h
