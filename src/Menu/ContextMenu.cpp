#include <algorithm>
#include <cassert>
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

	assert(ctx.renderer && "ContextMenu: renderer required before construction");
	int viewportCols = ctx.renderer->get_viewport_cols();
	int viewportRows = ctx.renderer->get_viewport_rows();

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

void ContextMenu::on_key(GameKey key, int ch, GameContext& ctx)
{
	const int maxIndex = static_cast<int>(menuOptions.size()) - 1;
	if (key == GameKey::UP)
	{
		if (selectedIndex > 0)
		{
			selectedIndex--;
		}
	}
	else if (key == GameKey::DOWN)
	{
		if (selectedIndex < maxIndex)
		{
			selectedIndex++;
		}
	}
	else if (key == GameKey::ENTER || key == GameKey::SPACE)
	{
		run = false;
		if (onSelect)
		{
			onSelect(selectedIndex, ctx);
		}
	}
	else if (key == GameKey::ESCAPE)
	{
		run = false;
		if (onSelect)
		{
			onSelect(-1, ctx);
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

	// Use inputSystem->get_key() instead of IsMouseButtonPressed() directly.
	// On Emscripten, poll() (called inside menu_key_listen()) consumes the
	// prev->curr transition for mouse buttons. A second raw IsMouseButtonPressed()
	// call in the same frame sees prev=curr=1 and returns false. Reading through
	// the InputSystem avoids the double-consumption.
	if (inputSystem && inputSystem->get_key() == GameKey::MOUSE_LEFT)
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

	on_key(lastKey, lastChar, ctx);

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
