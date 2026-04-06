#include <algorithm>
#include <string>

#include <raylib.h>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Gui/Gui.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/RenderingManager.h"
#include "../Utils/Vector2D.h"
#include "ContextMenu.h"

ContextMenu::ContextMenu(
	std::vector<std::string> options,
	int anchor_col,
	int anchor_row,
	std::function<void(int, GameContext&)> callback,
	GameContext& ctx)
	: menuOptions(std::move(options))
	, onSelect(std::move(callback))
{
	size_t longest = 0;
	for (const auto& opt : menuOptions)
	{
		longest = std::max(longest, opt.size());
	}

	int height = static_cast<int>(menuOptions.size()) + 3;
	int width = std::max(static_cast<int>(longest) + 4, 14);

	int viewportCols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 80;
	int viewportRows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 24;

	int startX = std::clamp(anchor_col, 0, std::max(0, viewportCols - width));
	int startY = std::clamp(anchor_row, 0, std::max(0, viewportRows - height));

	menu_new(
		static_cast<size_t>(width),
		static_cast<size_t>(height),
		static_cast<size_t>(startX),
		static_cast<size_t>(startY),
		ctx);
}

void ContextMenu::draw_content()
{
	menu_draw_box();
	menu_draw_title("Action", WHITE_BLACK_PAIR);

	for (int i = 0; i < static_cast<int>(menuOptions.size()); ++i)
	{
		if (i == selectedIndex)
		{
			menu_highlight_on();
		}
		menu_print(2, i + 2, menuOptions[static_cast<size_t>(i)]);
		if (i == selectedIndex)
		{
			menu_highlight_off();
		}
	}
}

// Called once per frame by MenuManager -- no blocking loop.
// Input is polled first (reads previous frame's PollInputEvents state),
// then we render. This matches InventoryUI's pattern.
void ContextMenu::menu(GameContext& ctx)
{
	// --- Input phase (before render, like InventoryUI) ---
	menu_key_listen();

	const int maxIndex = static_cast<int>(menuOptions.size()) - 1;

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		int sel = -1;
		int tileSize = renderer ? renderer->get_tile_size() : 16;
		Vector2D mousePos = inputSystem->get_mouse_tile(tileSize);
		int relRow = static_cast<int>(mousePos.y) - static_cast<int>(menuStartY);
		if (relRow >= 2 && relRow < 2 + static_cast<int>(menuOptions.size()))
		{
			sel = relRow - 2;
		}
		run = false;
		if (onSelect)
		{
			onSelect(sel, ctx);
		}
		return;
	}

	switch (keyPress)
	{

	case 0x103: // UP
	{
		if (selectedIndex > 0)
		{
			selectedIndex--;
		}
		break;
	}

	case 0x102: // DOWN
	{
		if (selectedIndex < maxIndex)
		{
			selectedIndex++;
		}
		break;
	}

	case '\n':
	case ' ':
	{
		run = false;
		if (onSelect)
		{
			onSelect(selectedIndex, ctx);
		}
		break;
	}

	case 27: // ESC
	{
		run = false;
		if (onSelect)
		{
			onSelect(-1, ctx);
		}
		break;
	}

	default:
	{
		break;
	}

	}

	if (!run)
	{
		return;
	}

	// --- Render phase ---
	if (renderer)
	{
		renderer->begin_frame();
	}
	if (ctx.renderingManager)
	{
		ctx.renderingManager->render(ctx);
	}
	if (ctx.gui && ctx.gui->guiInit)
	{
		ctx.gui->gui_render(ctx);
	}
	draw_content();
	menu_refresh();
}
