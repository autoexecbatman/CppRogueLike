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

#include "AbilityAllocation.h"
#include "BaseMenu.h"

struct GameContext;

class MenuAbilityScores : public BaseMenu
{
private:
	AbilityAllocation allocation;
	Ability cursor{ Ability::STRENGTH };

	void draw_allocation_screen();

	// Pixels between the frame's left and right borders, which every line is
	// measured against so none draws past the box.
	[[nodiscard]] int interior_width() const;

	// One ability's line: its name, what it is allocated, what the race will make
	// of it, and the marker when the cursor is on it.
	[[nodiscard]] std::string row_for(Ability ability) const;

	// The pool as the player picks from it, each die behind the digit that spends it.
	[[nodiscard]] std::string pool_line() const;

	// What the screen is waiting for: the minimum still unmet, or that it may be
	// accepted.
	[[nodiscard]] std::string status_line() const;

public:
	MenuAbilityScores(GameContext& ctx);
	MenuAbilityScores(const MenuAbilityScores&) = delete;
	MenuAbilityScores& operator=(const MenuAbilityScores&) = delete;
	MenuAbilityScores(MenuAbilityScores&&) = delete;
	MenuAbilityScores& operator=(MenuAbilityScores&&) = delete;

	void menu(GameContext& ctx) override;
};

// end of file: MenuAbilityScores.h
