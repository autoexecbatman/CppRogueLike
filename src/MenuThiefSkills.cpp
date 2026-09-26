// file: MenuThiefSkills.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <utility>

#include "Colors.h"
#include "GameContext.h"
#include "InputSystem.h"
#include "Player.h"
#include "Renderer.h"
#include "MenuThiefSkills.h"

namespace
{
// The box is sized to the viewport rather than fixed: a width in tiles that fits a
// 1920 screen puts startX negative on a 1280 one and the left half is never drawn.
constexpr int MENU_MARGIN_TILES = 2;
constexpr int MENU_MAX_WIDTH_TILES = 22;
constexpr int MENU_HEIGHT_TILES = 8;

// Text rows are 32 pixels, not a whole 64-pixel tile: the font is 16.
constexpr int TEXT_ROW_PITCH = 32;
constexpr int TEXT_TOP_INSET = 6;

// Rows, top to bottom: eight skills, the pool, the status, two hints.
constexpr int FIRST_SKILL_ROW = 0;
constexpr int POOL_ROW = 8;
constexpr int STATUS_ROW = 9;
constexpr int FIRST_HINT_ROW = 10;
} // namespace

void push_thief_skill_allocation(Player& character, int level, GameContext& ctx)
{
	// Only the one class the book gives thief skills to has anything to spend.
	if (character.get_creature_class() != CreatureClass::ROGUE)
	{
		return;
	}

	ThiefSkillAllocation allocation{
		character.thiefSkillPoints,
		thief_skill_grant_at_level(level),
		thief_skill_racial_adjustments(character.playerRaceState),
		character.get_dexterity(),
		// Armour Table 29 prints no column for adjusts nothing the table can name,
		// so the points are judged unarmoured rather than being stranded.
		character.thief_armor().value_or(ThiefArmor::NONE)
	};

	// Reached through the context's own handle rather than captured, so a screen
	// still open when the player is replaced writes to whoever the player is then.
	const auto keep_the_points = [](std::array<int, THIEF_SKILL_COUNT> points, GameContext& context)
	{
		context.player_concrete().thiefSkillPoints = points;
	};

	ctx.menus->push_back(std::make_unique<MenuThiefSkills>(ctx, std::move(allocation), keep_the_points));
}

MenuThiefSkills::MenuThiefSkills(
	GameContext& ctx,
	ThiefSkillAllocation startingAllocation,
	std::function<void(std::array<int, THIEF_SKILL_COUNT>, GameContext&)> acceptCallback)
	: allocation{ std::move(startingAllocation) }, onAccept{ std::move(acceptCallback) }
{
	assert(ctx.renderer && "MenuThiefSkills requires a renderer to size its box");
	assert(onAccept && "MenuThiefSkills requires somewhere to put the result");

	const int viewportCols = ctx.renderer->get_viewport_cols();
	const int viewportRows = ctx.renderer->get_viewport_rows();

	const int width = std::min(MENU_MAX_WIDTH_TILES, viewportCols - MENU_MARGIN_TILES * 2);
	assert(width > 0 && "MenuThiefSkills: viewport is too narrow to hold the box");

	menu_new(
		width,
		MENU_HEIGHT_TILES,
		(viewportCols - width) / 2,
		(viewportRows - MENU_HEIGHT_TILES) / 2,
		ctx);
}

int MenuThiefSkills::interior_width() const
{
	return (static_cast<int>(menuWidth) - 2) * renderer->get_tile_size();
}

std::string MenuThiefSkills::row_for(ThiefSkill skill) const
{
	const std::string marker = (cursor == skill) ? ">" : " ";
	const int assigned = allocation.assigned_to(skill);

	// What this grant put here is shown only where it put something, so a screen
	// nobody has touched yet is eight plain numbers.
	if (assigned == 0)
	{
		return std::format("{} {:<15}{:>3}%", marker, thief_skill_name(skill), allocation.score(skill));
	}
	return std::format(
		"{} {:<15}{:>3}% +{}",
		marker,
		thief_skill_name(skill),
		allocation.score(skill),
		assigned);
}

std::string MenuThiefSkills::pool_line() const
{
	return std::format("Points left: {}", allocation.remaining_points());
}

std::string MenuThiefSkills::status_line() const
{
	if (allocation.remaining_points() == 0)
	{
		return "All spent.";
	}
	return "Unspent points are lost.";
}

void MenuThiefSkills::draw_allocation_screen()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("THIEF SKILLS", YELLOW_BLACK_PAIR);

	assert(renderer && "MenuThiefSkills::draw_allocation_screen called before menu_new");
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

	int row = FIRST_SKILL_ROW;
	for (const ThiefSkill skill : ALL_THIEF_SKILL)
	{
		const int colorPair = (skill == cursor) ? YELLOW_BLACK_PAIR : WHITE_BLACK_PAIR;
		draw_row(row, row_for(skill), colorPair);
		++row;
	}

	draw_row(POOL_ROW, pool_line(), WHITE_BLACK_PAIR);

	const bool allSpent = allocation.remaining_points() == 0;
	draw_row(STATUS_ROW, status_line(), allSpent ? GREEN_BLACK_PAIR : RED_BLACK_PAIR);
	draw_row(FIRST_HINT_ROW, "[up/down] skill   [left/right] one point", CYAN_BLACK_PAIR);
	draw_row(FIRST_HINT_ROW + 1, "[space] fill it   [Enter] done", CYAN_BLACK_PAIR);

	menu_refresh();
}

void MenuThiefSkills::menu(GameContext& ctx)
{
	assert(inputSystem && "MenuThiefSkills::menu called before menu_new");
	inputSystem->poll();

	switch (inputSystem->get_key())
	{

	case GameKey::UP:
	{
		cursor = previous_thief_skill(cursor);
		break;
	}

	case GameKey::DOWN:
	{
		cursor = next_thief_skill(cursor);
		break;
	}

	case GameKey::RIGHT:
	{
		if (allocation.can_assign(cursor))
		{
			allocation.assign(cursor);
		}
		break;
	}

	case GameKey::LEFT:
	{
		if (allocation.can_take_back(cursor))
		{
			allocation.take_back(cursor);
		}
		break;
	}

	case GameKey::SPACE:
	{
		// Sixty points one keypress at a time is sixty keypresses, so one key puts
		// on as many as the grant, the per-skill limit and the 95 ceiling allow.
		while (allocation.can_assign(cursor))
		{
			allocation.assign(cursor);
		}
		break;
	}

	case GameKey::ENTER:
	{
		// The book leaves the distribution entirely to the player, so nothing here
		// refuses an acceptance - the status row says what is being left behind.
		menu_set_run_false();
		onAccept(allocation.total_points(), ctx);
		break;
	}

	default:
	{
		break;
	}
	}

	draw_allocation_screen();
}

// end of file: MenuThiefSkills.cpp
