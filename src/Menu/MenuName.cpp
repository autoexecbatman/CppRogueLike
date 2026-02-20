// file: MenuName.cpp
#include "MenuName.h"
#include "../Core/GameContext.h"
#include "../ActorTypes/Player.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"
#include "../Colors/Colors.h"
#include "../Systems/MenuManager.h"
#include "../Systems/RenderingManager.h"

MenuName::MenuName(GameContext& ctx)
{
	int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
	int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
	int w     = 32;
	int h     = 5;
	int sy    = (vrows - h) / 2;
	int sx    = (vcols - w) / 2;
	menu_new(h, w, sy, sx, ctx);
}

MenuName::~MenuName()
{
	menu_delete();
}

void MenuName::draw_name_screen(GameContext& ctx)
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("ENTER NAME", YELLOW_BLACK_PAIR);

	if (renderer)
	{
		int ts       = renderer->get_tile_size();
		int font_off = (ts - renderer->get_font_size()) / 2;
		int px       = (static_cast<int>(menu_startx) + 1) * ts;
		int row1_y   = (static_cast<int>(menu_starty) + 1) * ts + font_off;
		int row2_y   = (static_cast<int>(menu_starty) + 2) * ts + font_off;
		renderer->draw_text(px, row1_y, "Name: " + inputText + "_", WHITE_BLACK_PAIR);
		renderer->draw_text(px, row2_y, "[Enter] Confirm  [Esc] Skip", CYAN_BLACK_PAIR);
	}

	menu_refresh();
}

void MenuName::menu(GameContext& ctx)
{
	menu_name(ctx);
}

void MenuName::menu_name(GameContext& ctx)
{
	inputText.clear();
	run = true;

	while (run && !WindowShouldClose())
	{
		draw_name_screen(ctx);

		if (!input_system) break;

		input_system->poll();

		int ch      = input_system->get_char_input();
		GameKey gk  = input_system->get_key();

		if (ch >= 32 && ch < 127)
		{
			if (inputText.size() < 38)
				inputText += static_cast<char>(ch);
		}
		else if (ch == 8 || gk == GameKey::BACKSPACE)
		{
			if (!inputText.empty())
				inputText.pop_back();
		}
		else
		{
			switch (gk)
			{
			case GameKey::ENTER:
				ctx.player->actorData.name = inputText.empty() ? "Player" : inputText;
				menu_set_run_false();
				break;
			case GameKey::ESCAPE:
				ctx.player->actorData.name = "Player";
				menu_set_run_false();
				break;
			default:
				break;
			}
		}
	}

	if (ctx.menu_manager->is_game_initialized())
		ctx.rendering_manager->restore_game_display();
}

// end of file: MenuName.cpp
