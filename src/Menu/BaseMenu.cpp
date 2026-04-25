#include <cassert>
#include <string>

#include <raylib.h>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/TileConfig.h"
#include "BaseMenu.h"
#include "../Actor/Item.h"

void BaseMenu::menu_new(
	size_t width,
	size_t height,
	size_t startX,
	size_t startY,
	GameContext& ctx)
{
	menuWidth = width;
	menuHeight = height;
	menuStartX = startX;
	menuStartY = startY;
	assert(ctx.renderer && "BaseMenu::menu_new called without a renderer");
	assert(ctx.inputSystem && "BaseMenu::menu_new called without an inputSystem");
	assert(ctx.tileConfig && "BaseMenu::menu_new called without a tileConfig");
	renderer = ctx.renderer;
	inputSystem = ctx.inputSystem;
	tileConfig = ctx.tileConfig;
}

void BaseMenu::menu_clear()
{
	assert(renderer && "BaseMenu::menu_clear called without a renderer");
	renderer->begin_frame();
}

void BaseMenu::menu_print(int x, int y, const std::string& text)
{
	assert(renderer && "BaseMenu::menu_print called without a renderer");

	int tileSize = renderer->get_tile_size();
	int px = (static_cast<int>(menuStartX) + x) * tileSize;
	int py = (static_cast<int>(menuStartY) + y) * tileSize;
	int font_off = (tileSize - renderer->get_font_size()) / 2;

	if (isHighlighted)
	{
		int bar_x = (static_cast<int>(menuStartX) + 1) * tileSize;
		int bar_w = (static_cast<int>(menuWidth) - 2) * tileSize;
		ColorPair pair = renderer->get_color_pair(BLACK_WHITE_PAIR);
		DrawRectangle(bar_x, py, bar_w, tileSize, pair.bg);
		renderer->draw_text(Vector2D{ px, py + font_off }, text, BLACK_WHITE_PAIR);
	}
	else
	{
		renderer->draw_text(Vector2D{ px, py + font_off }, text, WHITE_BLACK_PAIR);
	}
}

void BaseMenu::menu_refresh()
{
	assert(renderer && "BaseMenu::menu_refresh called without a renderer");
	renderer->end_frame();
}

void BaseMenu::menu_key_listen()
{
	assert(inputSystem && "BaseMenu::menu_key_listen called without an inputSystem");

	inputSystem->poll();

	int ch = inputSystem->get_char_input();
	if (ch != 0)
	{
		keyPress = ch;
		return;
	}

	GameKey gk = inputSystem->get_key();
	switch (gk)
	{

	case GameKey::UP:
	{
		keyPress = 0x103;
		break;
	}

	case GameKey::DOWN:
	{
		keyPress = 0x102;
		break;
	}

	case GameKey::LEFT:
	{
		keyPress = 0x104;
		break;
	}

	case GameKey::RIGHT:
	{
		keyPress = 0x105;
		break;
	}

	case GameKey::ENTER:
	{
		keyPress = 10;
		break;
	}

	case GameKey::ESCAPE:
	{
		keyPress = 27;
		break;
	}

	case GameKey::TAB:
	{
		keyPress = 9;
		break;
	}

	case GameKey::SPACE:
	{
		keyPress = ' ';
		break;
	}

	case GameKey::BACKSPACE:
	{
		keyPress = 8;
		break;
	}

	default:
	{
		keyPress = 0;
		break;
	}
	}
}

void BaseMenu::menu_draw_box()
{
	assert(renderer && "BaseMenu::menu_draw_box called without a renderer");

	int tileSize = renderer->get_tile_size();
	int pixelX = static_cast<int>(menuStartX) * tileSize;
	int pixelY = static_cast<int>(menuStartY) * tileSize;

	renderer->draw_frame(Vector2D{ pixelX, pixelY }, static_cast<int>(menuWidth), static_cast<int>(menuHeight), *tileConfig);
}

void BaseMenu::menu_draw_title(std::string_view title, int colorPair)
{
	assert(renderer && "BaseMenu::menu_draw_title called without a renderer");

	int tileSize = renderer->get_tile_size();
	int fontOff = (tileSize - renderer->get_font_size()) / 2;
	int interiorWidth = (static_cast<int>(menuWidth) - 2) * tileSize;
	int textWidth = renderer->measure_text(title);
	int pixelX = (static_cast<int>(menuStartX) + 1) * tileSize + (interiorWidth - textWidth) / 2;
	int pixelY = static_cast<int>(menuStartY) * tileSize + fontOff;
	renderer->draw_text(Vector2D{ pixelX, pixelY }, title, colorPair);
}
