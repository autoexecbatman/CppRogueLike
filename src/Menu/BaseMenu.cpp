#include "BaseMenu.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"
#include "../Colors/Colors.h"

void BaseMenu::menu_new(
	size_t height,
	size_t width,
	size_t starty,
	size_t startx,
	GameContext& ctx
)
{
	menu_height = height;
	menu_width = width;
	menu_starty = starty;
	menu_startx = startx;
	renderer = ctx.renderer;
	input_system = ctx.input_system;
}

void BaseMenu::menu_clear()
{
	if (!renderer) return;
	renderer->begin_frame();
}

void BaseMenu::menu_print(int x, int y, const std::string& text)
{
	if (!renderer) return;

	int ts       = renderer->get_tile_size();
	int px       = (static_cast<int>(menu_startx) + x) * ts;
	int py       = (static_cast<int>(menu_starty) + y) * ts;
	int font_off = (ts - renderer->get_font_size()) / 2;

	if (isHighlighted)
	{
		int bar_x = (static_cast<int>(menu_startx) + 1) * ts;
		int bar_w = (static_cast<int>(menu_width) - 2) * ts;
		ColorPair pair = renderer->get_color_pair(BLACK_WHITE_PAIR);
		DrawRectangle(bar_x, py, bar_w, ts, pair.bg);
		renderer->draw_text(px, py + font_off, text, BLACK_WHITE_PAIR);
	}
	else
	{
		renderer->draw_text(px, py + font_off, text, WHITE_BLACK_PAIR);
	}
}

void BaseMenu::menu_refresh()
{
	if (!renderer) return;
	renderer->end_frame();
}

void BaseMenu::menu_key_listen()
{
	if (!input_system) return;

	input_system->poll();

	int ch = input_system->get_char_input();
	if (ch != 0)
	{
		keyPress = ch;
		return;
	}

	GameKey gk = input_system->get_key();
	switch (gk)
	{
	case GameKey::UP:        keyPress = 0x103; break;
	case GameKey::DOWN:      keyPress = 0x102; break;
	case GameKey::LEFT:      keyPress = 0x104; break;
	case GameKey::RIGHT:     keyPress = 0x105; break;
	case GameKey::ENTER:     keyPress = 10; break;
	case GameKey::ESCAPE:    keyPress = 27; break;
	case GameKey::TAB:       keyPress = 9; break;
	case GameKey::SPACE:     keyPress = ' '; break;
	case GameKey::BACKSPACE: keyPress = 8; break;
	default:                 keyPress = 0; break;
	}
}

void BaseMenu::menu_draw_box()
{
	if (!renderer) return;

	int ts = renderer->get_tile_size();
	int px = static_cast<int>(menu_startx) * ts;
	int py = static_cast<int>(menu_starty) * ts;

	renderer->draw_frame(px, py, static_cast<int>(menu_width), static_cast<int>(menu_height));
}

void BaseMenu::menu_draw_title(std::string_view title, int color_pair)
{
	if (!renderer) return;

	int ts         = renderer->get_tile_size();
	int font_off   = (ts - renderer->get_font_size()) / 2;
	int interior_w = (static_cast<int>(menu_width) - 2) * ts;
	int text_w     = renderer->measure_text(title);
	int px         = (static_cast<int>(menu_startx) + 1) * ts + (interior_w - text_w) / 2;
	int py         = static_cast<int>(menu_starty) * ts + font_off;
	renderer->draw_text(px, py, title, color_pair);
}
