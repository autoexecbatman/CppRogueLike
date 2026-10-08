// file: ListMenu.cpp
#include <algorithm>
#include <cassert>
#include <cctype>

#include <raylib.h>

#include "Colors.h"
#include "GameContext.h"
#include "InputSystem.h"
#include "ListMenu.h"
#include "Renderer.h"

ListMenu::ListMenu(
	std::string title,
	std::vector<MenuEntry> entries,
	std::function<void(GameContext&)> onEscape,
	std::function<void(GameContext&)> onFrame,
	GameContext& ctx)
	: title{ std::move(title) }, entries{ std::move(entries) }, onEscape{ std::move(onEscape) }, onFrame{ std::move(onFrame) }
{
	assert(ctx.renderer && "ListMenu: renderer required before construction");
	const int tileSize = ctx.renderer->get_tile_size();
	const int fontSize = ctx.renderer->get_font_size();

	// Sized from what the text measures, never from how many characters it has.
	// A character count handed to menu_new is read as a count of 64-pixel tiles.
	int widestText = ctx.renderer->measure_text(this->title);
	for (const auto& entry : this->entries)
	{
		widestText = std::max(widestText, ctx.renderer->measure_text(entry.label));
	}

	menuWidth = static_cast<size_t>(panel_tiles_for_text_width(widestText, tileSize));
	menuHeight = static_cast<size_t>(
		panel_tiles_for_text_rows(static_cast<int>(this->entries.size()), tileSize, fontSize));

	int vcols = ctx.renderer->get_viewport_cols();
	int vrows = ctx.renderer->get_viewport_rows();
	int startX = (vcols - static_cast<int>(menuWidth)) / 2;
	int startY = (vrows - static_cast<int>(menuHeight)) / 2;
	menuStartX = static_cast<size_t>(startX < 0 ? 0 : startX);
	menuStartY = static_cast<size_t>(startY < 0 ? 0 : startY);
	menu_new(menuWidth, menuHeight, menuStartX, menuStartY, ctx);
}

void ListMenu::draw_entries()
{
	for (size_t i = 0; i < entries.size(); ++i)
	{
		if (cursorIndex == i)
		{
			menu_highlight_on();
		}
		menu_print(1, static_cast<int>(i), entries[i].label);
		if (cursorIndex == i)
		{
			menu_highlight_off();
		}
	}
}

void ListMenu::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title(title, ColorPairId::YELLOW_BLACK);
	draw_entries();
	menu_refresh();
}

void ListMenu::on_key(GameContext& ctx)
{
	if (lastKey == GameKey::UP || lastKey == GameKey::W)
	{
		cursorIndex = (cursorIndex + entries.size() - 1) % entries.size();
	}
	else if (lastKey == GameKey::DOWN || lastKey == GameKey::S)
	{
		cursorIndex = (cursorIndex + 1) % entries.size();
	}
	else if (lastKey == GameKey::ENTER)
	{
		menu_set_run_false();
		if (entries[cursorIndex].command)
		{
			(*entries[cursorIndex].command)(ctx);
		}
	}
	else if (lastKey == GameKey::ESCAPE)
	{
		menu_set_run_false();
		if (onEscape)
		{
			onEscape(ctx);
		}
	}
	else
	{
		// Hotkey match, case-insensitive so 'M' and 'm' both work.
		for (auto& entry : entries)
		{
			if (entry.hotkey != 0 && std::tolower(lastChar) == std::tolower(entry.hotkey))
			{
				menu_set_run_false();
				if (entry.command)
				{
					(*entry.command)(ctx);
				}
				return;
			}
		}
	}
}

void ListMenu::menu(GameContext& ctx)
{
	if (onFrame)
	{
		onFrame(ctx);
	}
	menu_key_listen();

	// Hover follows the pointer, and only on a frame it moved: a stationary pointer
	// inside the panel would re-seat the cursor every frame and hide the arrow keys.
	if (menu_pointer_moved())
	{
		const ::Vector2 pointer = GetMousePosition();
		const auto hovered = menu_row_at(static_cast<int>(pointer.x), static_cast<int>(pointer.y), entries.size());
		if (hovered)
		{
			cursorIndex = *hovered;
		}
	}

	draw();

	if (menu_left_clicked())
	{
		const ::Vector2 pointer = GetMousePosition();
		const auto clicked = menu_row_at(static_cast<int>(pointer.x), static_cast<int>(pointer.y), entries.size());
		if (clicked)
		{
			menu_set_run_false();
			if (entries[*clicked].command)
			{
				(*entries[*clicked].command)(ctx);
			}
		}

		// A click beside the panel is ignored rather than treated as a cancel. onEscape
		// belongs to the ESC key; a misclick must never close or quit.
		return;
	}

	on_key(ctx);
}

// end of file: ListMenu.cpp
