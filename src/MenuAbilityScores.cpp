// file: MenuAbilityScores.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <string>

#include "Colors.h"
#include "GameContext.h"
#include "InputSystem.h"
#include "MenuName.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "MenuAbilityScores.h"

namespace
{
// The box is sized to the viewport rather than fixed: a width in tiles that fits a
// 1920 screen puts startX negative on a 1280 one and the left half is never drawn.
constexpr int MENU_MARGIN_TILES = 2;
constexpr int MENU_MAX_WIDTH_TILES = 20;
constexpr int MENU_HEIGHT_TILES = 8;

// Text rows are 32 pixels, not a whole 64-pixel tile: the font is 16.
constexpr int TEXT_ROW_PITCH = 32;
constexpr int TEXT_TOP_INSET = 6;

// Rows, top to bottom: six abilities, a blank, the pool, the status, two hints.
// The hints are two rows because one line of them does not fit the box's width and
// fit_text_to_width cuts the last key off rather than wrapping it.
constexpr int FIRST_ABILITY_ROW = 0;
constexpr int POOL_ROW = 7;
constexpr int STATUS_ROW = 8;
constexpr int FIRST_HINT_ROW = 9;

// The digits that spend a die. '1' takes the first die of the pool.
constexpr int FIRST_DIE_CHARACTER = '1';
} // namespace

MenuAbilityScores::MenuAbilityScores(GameContext& ctx)
	: allocation(
		  roll_method_six_pool(*ctx.dice),
		  ctx.playerBlueprint->creatureClass,
		  ctx.playerBlueprint->racialModifier)
{
	assert(ctx.renderer && "MenuAbilityScores requires a renderer to size its box");
	assert(ctx.playerBlueprint && "MenuAbilityScores requires the blueprint the menus fill");

	const int viewportCols = ctx.renderer->get_viewport_cols();
	const int viewportRows = ctx.renderer->get_viewport_rows();

	const int width = std::min(MENU_MAX_WIDTH_TILES, viewportCols - MENU_MARGIN_TILES * 2);
	assert(width > 0 && "MenuAbilityScores: viewport is too narrow to hold the box");

	menu_new(
		width,
		MENU_HEIGHT_TILES,
		(viewportCols - width) / 2,
		(viewportRows - MENU_HEIGHT_TILES) / 2,
		ctx);
}

int MenuAbilityScores::interior_width() const
{
	return (static_cast<int>(menuWidth) - 2) * renderer->get_tile_size();
}

std::string MenuAbilityScores::row_for(Ability ability) const
{
	const int allocated = allocation.allocated(ability);
	const int modifier = allocation.score(ability) - allocated;
	const std::string marker = (ability == cursor) ? ">" : " ";

	// The race is shown only where it does something, so five of six lines stay quiet.
	if (modifier == 0)
	{
		return std::format("{} {:<13}{:>3}", marker, ability_name(ability), allocated);
	}
	return std::format(
		"{} {:<13}{:>3} {:+} = {}",
		marker,
		ability_name(ability),
		allocated,
		modifier,
		allocation.score(ability));
}

std::string MenuAbilityScores::pool_line() const
{
	if (allocation.pool().empty())
	{
		return "Dice: none left";
	}

	std::string line = "Dice:";
	for (std::size_t index = 0; index < allocation.pool().size(); ++index)
	{
		line += std::format(" {}){}", index + 1, allocation.pool().at(index));
	}
	return line;
}

std::string MenuAbilityScores::status_line() const
{
	const std::optional<ClassMinimum> shortfall = allocation.unmet_minimum();
	if (!shortfall.has_value())
	{
		return "Ready.";
	}
	return std::format("Needs {} {}", ability_name(shortfall->ability), shortfall->score);
}

void MenuAbilityScores::draw_allocation_screen()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("ABILITY SCORES", YELLOW_BLACK_PAIR);

	assert(renderer && "MenuAbilityScores::draw_allocation_screen called before menu_new");
	const int tileSize = renderer->get_tile_size();
	const int textX = (static_cast<int>(menuStartX) + 1) * tileSize;
	const int firstRowY = static_cast<int>(menuStartY) * tileSize + tileSize + TEXT_TOP_INSET;

	const auto draw_row = [this, textX, firstRowY](int row, const std::string& text, int colorPair)
	{
		renderer->draw_text(
			Vector2D{ textX, firstRowY + row * TEXT_ROW_PITCH },
			renderer->fit_text_to_width(text, interior_width()),
			colorPair);
	};

	int row = FIRST_ABILITY_ROW;
	for (const Ability ability : ALL_ABILITY)
	{
		const int colorPair = (ability == cursor) ? YELLOW_BLACK_PAIR : WHITE_BLACK_PAIR;
		draw_row(row, row_for(ability), colorPair);
		++row;
	}

	draw_row(POOL_ROW, pool_line(), WHITE_BLACK_PAIR);

	const bool ready = !allocation.unmet_minimum().has_value();
	draw_row(STATUS_ROW, status_line(), ready ? GREEN_BLACK_PAIR : RED_BLACK_PAIR);
	draw_row(FIRST_HINT_ROW, "[up/down] choose an ability   [1-7] spend that die", CYAN_BLACK_PAIR);
	draw_row(FIRST_HINT_ROW + 1, "[backspace] take a die back   [Enter] done", CYAN_BLACK_PAIR);

	menu_refresh();
}

void MenuAbilityScores::menu(GameContext& ctx)
{
	assert(inputSystem && "MenuAbilityScores::menu called before menu_new");
	inputSystem->poll();

	const int charInput = inputSystem->get_char_input();
	const GameKey gameKey = inputSystem->get_key();

	// A digit names a die of the pool, which is the only thing a number means here.
	const int dieNumber = charInput - FIRST_DIE_CHARACTER;
	if (dieNumber >= 0 && dieNumber < METHOD_SIX_DICE)
	{
		const std::size_t dieIndex = static_cast<std::size_t>(dieNumber);
		if (allocation.can_spend(cursor, dieIndex))
		{
			allocation.spend(cursor, dieIndex);
		}
	}
	else
	{
		switch (gameKey)
		{

		case GameKey::UP:
		{
			cursor = previous_ability(cursor);
			break;
		}

		case GameKey::DOWN:
		{
			cursor = next_ability(cursor);
			break;
		}

		case GameKey::BACKSPACE:
		{
			if (allocation.has_placed(cursor))
			{
				allocation.take_back(cursor);
			}
			break;
		}

		case GameKey::ENTER:
		{
			// The class minimum is the one thing the screen will not let past; the
			// book leaves spending every die to the player.
			if (!allocation.unmet_minimum().has_value())
			{
				ctx.playerBlueprint->abilityScores = allocation.allocated_scores();
				menu_set_run_false();
				ctx.menus->push_back(std::make_unique<MenuName>(ctx));
			}
			break;
		}

		default:
		{
			break;
		}
		}
	}

	draw_allocation_screen();
}

// end of file: MenuAbilityScores.cpp
