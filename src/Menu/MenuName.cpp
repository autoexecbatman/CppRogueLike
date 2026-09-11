#include <algorithm>
#include <cassert>
#include <string>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/RenderingManager.h"
#include "MenuName.h"

// The box is sized to the viewport. A fixed 32 tiles is 2048 pixels on a 1280
// pixel screen, which put startX at -6 tiles: the hint line was drawn from -320
// and the screen showed only its tail, "sc] Skip".
static constexpr int MENU_MARGIN_TILES = 2;
static constexpr int MENU_MAX_W_TILES = 16;
static constexpr int MENU_H_TILES = 4;
// Text rows are 32 pixels, not a whole 64-pixel tile: the font is 16.
static constexpr int TEXT_ROW_PITCH = 32;
// Gap between the frame's top edge and the first row of text.
static constexpr int TEXT_TOP_INSET = 6;

MenuName::MenuName(GameContext& ctx)
{
	assert(ctx.renderer && "MenuName requires a renderer to size its box");
	const int viewportCols = ctx.renderer->get_viewport_cols();
	const int viewportRows = ctx.renderer->get_viewport_rows();

	const int width = std::min(MENU_MAX_W_TILES, viewportCols - MENU_MARGIN_TILES * 2);
	assert(width > 0 && "MenuName: viewport is too narrow to hold the name box");

	menu_new(
		width,
		MENU_H_TILES,
		(viewportCols - width) / 2,
		(viewportRows - MENU_H_TILES) / 2,
		ctx);
}

// Pixels available for text between the frame's left and right borders.
int MenuName::interior_width() const
{
	return (static_cast<int>(menuWidth) - 2) * renderer->get_tile_size();
}

void MenuName::draw_name_screen()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("ENTER NAME", YELLOW_BLACK_PAIR);

	assert(renderer && "MenuName::draw_name_screen called before menu_new");
	const int tileSize = renderer->get_tile_size();
	const int textX = (static_cast<int>(menuStartX) + 1) * tileSize;
	const int firstRowY = static_cast<int>(menuStartY) * tileSize + tileSize + TEXT_TOP_INSET;

	renderer->draw_text(Vector2D{ textX, firstRowY }, "Name: " + inputText + "_", WHITE_BLACK_PAIR);
	renderer->draw_text(
		Vector2D{ textX, firstRowY + TEXT_ROW_PITCH },
		renderer->fit_text_to_width("[Enter] Confirm  [Esc] Skip", interior_width()),
		CYAN_BLACK_PAIR);

	menu_refresh();
}

void MenuName::menu(GameContext& ctx)
{
	if (!initialized)
	{
		inputText.clear();
		initialized = true;
	}
	assert(inputSystem && "MenuName::menu called before menu_new");
	inputSystem->poll();

	int charInput = inputSystem->get_char_input();
	GameKey gameKey = inputSystem->get_key();

	if (charInput >= 32 && charInput < 127)
	{
		// The field stops at the edge of its box rather than at a character count,
		// so a wide name cannot draw past the frame.
		const std::string candidate = inputText + static_cast<char>(charInput);
		if (renderer->measure_text("Name: " + candidate + "_") <= interior_width())
		{
			inputText = candidate;
		}
	}
	else if (charInput == 8 || gameKey == GameKey::BACKSPACE)
	{
		if (!inputText.empty())
		{
			inputText.pop_back();
		}
	}
	else
	{
		switch (gameKey)
		{

		case GameKey::ENTER:
		{
			ctx.playerBlueprint->name = inputText.empty() ? "Player" : inputText;
			menu_set_run_false();
			break;
		}

		case GameKey::ESCAPE:
		{
			ctx.playerBlueprint->name = "Player";
			menu_set_run_false();
			break;
		}

		default:
		{
			break;
		}
		}
	}

	draw_name_screen();

	if (!run && !ctx.menuManager->is_game_initialized())
	{
		ctx.stateManager->init_new_game(ctx);
		ctx.menuManager->set_game_initialized(true);
	}

	if (!run && ctx.menuManager->is_game_initialized())
	{
		ctx.renderingManager->restore_game_display();
	}
}
