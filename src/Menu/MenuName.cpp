#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/RenderingManager.h"
#include "MenuName.h"

MenuName::MenuName(GameContext& ctx)
{
	int viewportRows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
	int viewportCols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
	int width = 32;
	int height = 5;
	int startX = (viewportCols - width) / 2;
	int startY = (viewportRows - height) / 2;
	menu_new(
		width,
		height,
		startX,
		startY,
		ctx);
}

// TODO: menu_delete() is a stub. A code smell. Makes us declare a destructor that is empty while it should be compiler generated.
MenuName::~MenuName()
{
	menu_delete();
}

void MenuName::draw_name_screen(GameContext& ctx)
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("ENTER NAME", YELLOW_BLACK_PAIR);

	// TODO: What does it mean for renderer to be null? Should it be an assertion instead?
	if (renderer)
	{
		int tileSize = renderer->get_tile_size();
		int fontOff = (tileSize - renderer->get_font_size()) / 2;
		int pixelX = (static_cast<int>(menuStartX) + 1) * tileSize;
		int rowY1 = (static_cast<int>(menuStartY) + 1) * tileSize + fontOff;
		int rowY2 = (static_cast<int>(menuStartY) + 2) * tileSize + fontOff;
		renderer->draw_text(Vector2D{ pixelX, rowY1 }, "Name: " + inputText + "_", WHITE_BLACK_PAIR);
		renderer->draw_text(Vector2D{ pixelX, rowY2 }, "[Enter] Confirm  [Esc] Skip", CYAN_BLACK_PAIR);
	}

	menu_refresh();
}

void MenuName::menu(GameContext& ctx)
{
	if (!initialized)
	{
		inputText.clear();
		initialized = true;
	}
	// TODO: What does it mean for inputSystem to be null? Should it be an assertion instead?
	if (!inputSystem)
	{
		run = false;
		return;
	}

	inputSystem->poll();

	int charInput = inputSystem->get_char_input();
	GameKey gameKey = inputSystem->get_key();

	if (charInput >= 32 && charInput < 127)
	{
		if (inputText.size() < 38)
		{
			inputText += static_cast<char>(charInput);
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

	draw_name_screen(ctx);

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
