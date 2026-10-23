#include <algorithm>
#include <cassert>
#include <string>

#include <raylib.h>

#include "Colors.h"
#include "ContextMenu.h"
#include "GameContext.h"
#include "Gui.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "RenderingManager.h"

ContextMenu::ContextMenu(
	std::vector<std::string> options,
	int anchor_col,
	int anchor_row,
	std::function<void(int, GameContext&)> callback,
	GameContext& ctx) :
	menuOptions(std::move(options)),
	onSelect(std::move(callback))
{
	assert(
		ctx.renderer && "ContextMenu: renderer required before construction");
	const int tileSize = ctx.renderer->get_tile_size();

	// Measured, not counted. As a character count this was at least 14, which
	// menu_new reads as 14 tiles - 896 pixels for a menu of one-word options.
	int widestText = 0;
	for (const auto& opt : menuOptions)
	{
		widestText = std::max(widestText, ctx.renderer->measure_text(opt));
	}

	int width = panel_tiles_for_text_width(widestText, tileSize);
	int height = panel_tiles_for_text_rows(
		static_cast<int>(menuOptions.size()),
		tileSize,
		ctx.renderer->get_font_size());

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
	menu_draw_title("Action", ColorPairId::WHITE_BLACK);

	for (
		int optionIndex = 0; optionIndex < static_cast<int>(menuOptions.size());
		++optionIndex)
	{
		if (optionIndex == selectedIndex)
		{
			menu_highlight_on();
		}
		menu_print(
			1,
			optionIndex,
			menuOptions[static_cast<size_t>(optionIndex)]);
		if (optionIndex == selectedIndex)
		{
			menu_highlight_off();
		}
	}
}

void ContextMenu::on_key(GameContext& ctx)
{
	const int maxIndex = static_cast<int>(menuOptions.size()) - 1;
	if (lastKey == GameKey::UP)
	{
		if (selectedIndex > 0)
		{
			selectedIndex--;
		}
	}
	else if (lastKey == GameKey::DOWN)
	{
		if (selectedIndex < maxIndex)
		{
			selectedIndex++;
		}
	}
	else if (lastKey == GameKey::ENTER || lastKey == GameKey::SPACE)
	{
		run = false;
		if (onSelect)
		{
			onSelect(selectedIndex, ctx);
		}
	}
	else if (lastKey == GameKey::ESCAPE)
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
	assert(ctx.gui && "ContextMenu::menu: called without a gui");

	// --- Input phase (before render, like InventoryUI) ---
	menu_key_listen();

	// Read the click through inputSystem: poll() inside menu_key_listen() has already consumed
	// the button's transition, so a raw IsMouseButtonPressed() here returns false.
	if (inputSystem && inputSystem->get_key() == GameKey::MOUSE_LEFT)
	{
		int sel = -1;
		int tileSize = renderer ? renderer->get_tile_size() : 16;
		// Options are menu_print rows a pitch apart and the title sits on the frame, so the row under
		// the pointer is the option, with no offset.
		::Vector2 rawMouse = GetMousePosition();
		int relRow = panel_text_row_at_y(
			static_cast<int>(menuStartY) * tileSize,
			tileSize,
			static_cast<int>(rawMouse.y));
		if (relRow >= 0 && relRow < static_cast<int>(menuOptions.size()))
		{
			sel = relRow;
		}
		run = false;
		if (onSelect)
		{
			onSelect(sel, ctx);
		}
		return;
	}

	on_key(ctx);

	if (!run)
	{
		return;
	}

	// --- Render phase ---
	assert(
		ctx.renderingManager &&
		"ContextMenu::menu: called without a renderingManager");
	ctx.renderingManager->render(ctx);
	if (ctx.gui->guiInit)
	{
		ctx.gui->gui_render(ctx);
	}
	draw_content();
}
