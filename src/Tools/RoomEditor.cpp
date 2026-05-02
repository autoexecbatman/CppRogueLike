// file: RoomEditor.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include <raylib.h>

#include "../Actor/Item.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Menu/Menu.h"
#include "../Renderer/Renderer.h" // includes RaylibIncludes.h -> raylib.h + undefs
#include "../Systems/TileConfig.h"
#include "PrefabLibrary.h"
#include "RoomEditor.h"

// Layout (all in tiles; pixels = value * tile_size)
constexpr int LEFT_W_TILES = 11;
constexpr int RIGHT_W_TILES = 13;
constexpr int TOP_H_TILES = 1;
constexpr int BOT_H_TILES = 1;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RoomEditor::enter(PrefabLibrary& lib)
{
	library = &lib;
	active = true;
	mode = EditorMode::NORMAL;
	inputBuffer.clear();
	listSelected = -1;
	listScroll = 0;
	statusMsg.clear();
	new_canvas(canvasWidth, canvasHeight);
}

void RoomEditor::exit(GameContext& ctx)
{
	active = false;
	// Return to main menu
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void RoomEditor::tick(GameContext& ctx)
{
	tileConfig = ctx.tileConfig;
	handle_input(ctx);

	ctx.renderer->begin_frame();
	render(ctx);
	ctx.renderer->end_frame();
}

// ---------------------------------------------------------------------------
// Canvas management
// ---------------------------------------------------------------------------

void RoomEditor::new_canvas(int width, int height)
{
	canvasWidth = std::max(3, width);
	canvasHeight = std::max(3, height);
	canvas.assign(canvasHeight, std::string(canvasWidth, '.'));
	decorCanvas.assign(canvasHeight, std::string(canvasWidth, ' '));

	// Fill border with walls
	for (int col = 0; col < canvasWidth; ++col)
	{
		canvas[0][col] = '#';
		canvas[canvasHeight - 1][col] = '#';
	}
	for (int row = 0; row < canvasHeight; ++row)
	{
		canvas[row][0] = '#';
		canvas[row][canvasWidth - 1] = '#';
	}

	panX = 0;
	panY = 0;
}

void RoomEditor::do_save(const std::string& name)
{
	if (!library)
	{
		return;
	}

	Prefab p;
	p.name = name.empty() ? std::format("prefab_{}x{}", canvasWidth, canvasHeight) : name;
	p.depthMin = depthMin;
	p.depthMax = depthMax;
	p.weight = weight;

	// Merge base + decor layers: decor symbol wins when present.
	p.rows.clear();
	p.rows.reserve(canvasHeight);
	for (int row = 0; row < canvasHeight; ++row)
	{
		std::string rowStr;
		rowStr.reserve(canvasWidth);
		for (int col = 0; col < canvasWidth; ++col)
		{
			char decorChar = (row < static_cast<int>(decorCanvas.size()) &&
						 col < static_cast<int>(decorCanvas[row].size()))
				? decorCanvas[row][col]
				: ' ';

			char baseChar = (row < static_cast<int>(canvas.size()) &&
						 col < static_cast<int>(canvas[row].size()))
				? canvas[row][col]
				: '.';

			rowStr += (decorChar != ' ') ? decorChar : baseChar;
		}
		p.rows.push_back(std::move(rowStr));
	}

	prefabName = p.name;
	library->add_or_replace(std::move(p));
	library->save(Paths::PREFABS);
	set_status(std::format("Saved: {}", prefabName));
}

void RoomEditor::do_load(int prefabIndex)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefabIndex < 0 || prefabIndex >= static_cast<int>(all.size()))
	{
		return;
	}

	const Prefab& p = all[prefabIndex];
	prefabName = p.name;
	depthMin = p.depthMin;
	depthMax = p.depthMax;
	weight = p.weight;

	canvasHeight = static_cast<int>(p.rows.size());
	canvasWidth = canvasHeight > 0 ? static_cast<int>(p.rows[0].size()) : 0;
	canvas.assign(canvasHeight, std::string(canvasWidth, '.'));
	decorCanvas.assign(canvasHeight, std::string(canvasWidth, ' '));

	auto is_structural_sym = [](char sym) -> bool
	{
		return sym == '#' || sym == '.' || sym == ',' || sym == '+' || sym == '~';
	};

	for (int row = 0; row < canvasHeight; ++row)
	{
		const std::string& srcRow = p.rows[row];
		for (int col = 0; col < static_cast<int>(srcRow.size()) && col < canvasWidth; ++col)
		{
			char sym = srcRow[col];
			if (is_structural_sym(sym))
			{
				canvas[row][col] = sym;
				decorCanvas[row][col] = ' ';
			}
			else
			{
				canvas[row][col] = '.';
				decorCanvas[row][col] = sym;
			}
		}
	}

	panX = 0;
	panY = 0;
	set_status(std::format("Loaded: {}", prefabName));
}

void RoomEditor::do_delete(int prefabIndex)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefabIndex < 0 || prefabIndex >= static_cast<int>(all.size()))
	{
		return;
	}

	std::string name = all[prefabIndex].name;
	library->remove(name);
	library->save(Paths::PREFABS);
	listSelected = -1;
	set_status(std::format("Deleted: {}", name));
}

void RoomEditor::set_status(const std::string& msg)
{
	statusMsg = msg;
	statusTime = GetTime();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void RoomEditor::handle_input(GameContext& ctx)
{
	if (mode == EditorMode::TILE_PICKER)
	{
		handle_input_picker(ctx);
	}
	else if (mode == EditorMode::NORMAL)
	{
		handle_input_normal(ctx);
	}
	else
	{
		handle_input_text(ctx);
	}
}

void RoomEditor::handle_input_normal(GameContext& ctx)
{
	Renderer& renderer = *ctx.renderer;
	int tileSize = renderer.get_tile_size();

	// Escape -- exit
	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	// Global zoom
	if (IsKeyPressed(KEY_EQUAL))
	{
		renderer.zoom_in();
		return;
	}
	if (IsKeyPressed(KEY_MINUS))
	{
		renderer.zoom_out();
		return;
	}

	// F2 -- open tile picker for current palette symbol
	if (IsKeyPressed(KEY_F2) && library)
	{
		pickerSheetListIndex = 0;
		pickerCol = 0;
		pickerRow = 0;
		mode = EditorMode::TILE_PICKER;
		return;
	}

	// F3 -- rename label for current palette symbol
	if (IsKeyPressed(KEY_F3) && library)
	{
		const auto& pal = library->ordered_palette();
		if (paletteIndex >= 0 && paletteIndex < static_cast<int>(pal.size()))
		{
			inputBuffer = pal[paletteIndex].label;
			mode = EditorMode::INPUT_LABEL;
		}
		return;
	}

	// Palette navigation
	const auto& pal = library->ordered_palette();
	int palSize = static_cast<int>(pal.size());

	if (IsKeyPressed(KEY_UP))
	{
		paletteIndex = (paletteIndex - 1 + palSize) % palSize;
	}
	if (IsKeyPressed(KEY_DOWN))
	{
		paletteIndex = (paletteIndex + 1) % palSize;
	}

	// New canvas
	if (IsKeyPressed(KEY_N))
	{
		mode = EditorMode::INPUT_WIDTH;
		inputBuffer.clear();
		return;
	}

	// Delete selected prefab
	if (IsKeyPressed(KEY_DELETE) && listSelected >= 0)
	{
		do_delete(listSelected);
		return;
	}

	// Load selected prefab
	if (IsKeyPressed(KEY_ENTER) && listSelected >= 0)
	{
		do_load(listSelected);
		return;
	}

	// Ctrl+S -- initiate save (enter name)
	if (IsKeyPressed(KEY_S) &&
		(IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
	{
		mode = EditorMode::INPUT_NAME;
		inputBuffer = prefabName;
		return;
	}

	// Depth range (Ctrl+Up / Ctrl+Down adjusts depth_min/max)
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	if (ctrl && IsKeyPressed(KEY_UP))
	{
		depthMin = std::max(1, depthMin - 1);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_DOWN))
	{
		depthMin = std::min(depthMin + 1, depthMax);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_RIGHT))
	{
		depthMax = std::min(99, depthMax + 1);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_LEFT))
	{
		depthMax = std::max(depthMin, depthMax - 1);
		return;
	}

	// Middle mouse -- pan
	if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
	{
		if (!panning)
		{
			panning = true;
			::Vector2 mp = GetMousePosition();
			panMouseStartX = mp.x;
			panMouseStartY = mp.y;
			panStartX = panX;
			panStartY = panY;
		}
		else
		{
			::Vector2 mp = GetMousePosition();
			panX = panStartX - static_cast<int>(mp.x - panMouseStartX);
			panY = panStartY - static_cast<int>(mp.y - panMouseStartY);
		}
	}
	else
	{
		panning = false;
	}

	// Mouse wheel -- scroll palette (left panel) or prefab list (right panel)
	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f && library)
	{
		::Vector2 mw = GetMousePosition();
		int delta = (wheel > 0.0f) ? -1 : 1;

		// TODO: This if statement should be a named lambda for self-documenting code clarity.
		if (screen_to_palette_index(renderer, static_cast<int>(mw.x), static_cast<int>(mw.y)).has_value() ||
			static_cast<int>(mw.x) < LEFT_W_TILES * tileSize)
		{
			int palSize = static_cast<int>(library->ordered_palette().size());
			paletteIndex = std::clamp(paletteIndex + delta, 0, std::max(0, palSize - 1));
		}
		else
		{
			int count = static_cast<int>(library->all().size());
			listScroll = std::clamp(listScroll + delta, 0, std::max(0, count - 1));
		}
	}

	// Left click -- paint on canvas or select palette / prefab list entry
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		::Vector2 mp = GetMousePosition();
		int cx, cy;

		// Left panel -- select palette symbol
		auto palIdx = screen_to_palette_index(renderer, static_cast<int>(mp.x), static_cast<int>(mp.y));
		if (palIdx)
		{
			paletteIndex = *palIdx;
			return;
		}

		// Canvas
		if (screen_to_canvas(renderer, static_cast<int>(mp.x), static_cast<int>(mp.y), cx, cy))
		{
			char sym = selected_sym();
			bool structural = (sym == '#' || sym == '.' || sym == ',' ||
				sym == '+' || sym == '~');
			if (structural)
			{
				canvas[cy][cx] = sym;
				decorCanvas[cy][cx] = ' '; // clear any decor overlay
			}
			else
			{
				decorCanvas[cy][cx] = sym; // layer over the base tile
			}
			return;
		}

		// Right panel list
		auto listIdx = screen_to_list_index(renderer, static_cast<int>(mp.x), static_cast<int>(mp.y));
		if (listIdx)
		{
			listSelected = *listIdx;
			return;
		}
	}

	// Right click -- erase on canvas
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		::Vector2 mp = GetMousePosition();
		int cx, cy;
		if (screen_to_canvas(renderer, static_cast<int>(mp.x), static_cast<int>(mp.y), cx, cy))
		{
			// First erase decor overlay; if already empty, erase the base tile.
			if (decorCanvas[cy][cx] != ' ')
			{
				decorCanvas[cy][cx] = ' ';
			}
			else
			{
				bool onBorder = (cx == 0 || cx == canvasWidth - 1 || cy == 0 || cy == canvasHeight - 1);
				canvas[cy][cx] = onBorder ? '#' : '.';
			}
		}
	}
}

void RoomEditor::handle_input_text(GameContext& ctx)
{
	// Backspace
	if (IsKeyPressed(KEY_BACKSPACE) && !inputBuffer.empty())
	{
		inputBuffer.pop_back();
	}

	// Escape -- cancel
	if (IsKeyPressed(KEY_ESCAPE))
	{
		mode = EditorMode::NORMAL;
		inputBuffer.clear();
		return;
	}

	// Enter -- confirm
	if (IsKeyPressed(KEY_ENTER))
	{
		if (mode == EditorMode::INPUT_WIDTH)
		{
			pendingWidth = inputBuffer.empty() ? canvasWidth : std::stoi(inputBuffer);
			pendingWidth = std::clamp(pendingWidth, 3, 60);
			mode = EditorMode::INPUT_HEIGHT;
			inputBuffer.clear();
		}
		else if (mode == EditorMode::INPUT_HEIGHT)
		{
			int newHeight = inputBuffer.empty() ? canvasHeight : std::stoi(inputBuffer);
			newHeight = std::clamp(newHeight, 3, 40);
			new_canvas(pendingWidth, newHeight);
			mode = EditorMode::NORMAL;
			inputBuffer.clear();
		}
		else if (mode == EditorMode::INPUT_NAME)
		{
			do_save(inputBuffer);
			mode = EditorMode::NORMAL;
			inputBuffer.clear();
		}
		else if (mode == EditorMode::INPUT_LABEL)
		{
			if (library && !inputBuffer.empty())
			{
				const auto& pal = library->ordered_palette();
				if (paletteIndex >= 0 && paletteIndex < static_cast<int>(pal.size()))
				{
					char sym = pal[paletteIndex].symbol;
					library->set_symbol_label(sym, inputBuffer);
					library->save(Paths::PREFABS);
					set_status(std::format("Label: '{}'={}", sym, inputBuffer));
				}
			}
			mode = EditorMode::NORMAL;
			inputBuffer.clear();
		}
		return;
	}

	// Character input (digits for size modes, any printable for name)
	int ch = GetCharPressed();
	while (ch > 0)
	{
		bool accept = false;
		if (mode == EditorMode::INPUT_WIDTH || mode == EditorMode::INPUT_HEIGHT)
		{
			accept = (ch >= '0' && ch <= '9') && inputBuffer.size() < 3;
		}
		else
		{
			accept = (ch >= 32 && ch <= 126) && inputBuffer.size() < 40;
		}
		// INPUT_LABEL uses the same printable-character rule as INPUT_NAME

		if (accept)
		{
			inputBuffer += static_cast<char>(ch);
		}

		ch = GetCharPressed();
	}
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

// Returns canvas cell coords from screen pixel, false if outside canvas.
bool RoomEditor::screen_to_canvas(
	const Renderer& renderer,
	int mousePx,
	int mousePy,
	int& outCx,
	int& outCy) const
{
	int tileSize = renderer.get_tile_size();
	int areaX = LEFT_W_TILES * tileSize;
	int areaY = TOP_H_TILES * tileSize;
	int areaW = renderer.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * tileSize;
	int areaH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mousePx < areaX || mousePx >= areaX + areaW)
	{
		return false;
	}
	if (mousePy < areaY || mousePy >= areaY + areaH)
	{
		return false;
	}

	int relX = mousePx - areaX + panX;
	int relY = mousePy - areaY + panY;

	outCx = relX / tileSize;
	outCy = relY / tileSize;

	if (outCx < 0 || outCx >= canvasWidth)
	{
		return false;
	}
	if (outCy < 0 || outCy >= canvasHeight)
	{
		return false;
	}

	return true;
}

// Returns prefab list index from screen pixel in right panel. Returns nullopt if outside panel.
std::optional<int> RoomEditor::screen_to_list_index(const Renderer& renderer, int mousePx, int mousePy) const
{
	if (!library)
	{
		return std::nullopt;
	}

	int tileSize = renderer.get_tile_size();
	int panelX = renderer.get_screen_width() - RIGHT_W_TILES * tileSize;
	int panelY = TOP_H_TILES * tileSize;
	int panelH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mousePx < panelX || mousePx >= renderer.get_screen_width())
	{
		return std::nullopt;
	}
	if (mousePy < panelY || mousePy >= panelY + panelH)
	{
		return std::nullopt;
	}

	int fontH = renderer.get_font_size() + 2;
	int entryCount = static_cast<int>(library->all().size());
	int visible = panelH / fontH;
	int row = (mousePy - panelY) / fontH;
	int idx = listScroll + row;

	if (idx < 0 || idx >= entryCount || row >= visible)
	{
		return std::nullopt;
	}

	return idx;
}

// Returns palette index from screen pixel in left panel. Returns nullopt if outside panel.
std::optional<int> RoomEditor::screen_to_palette_index(const Renderer& renderer, int mousePx, int mousePy) const
{
	if (!library)
	{
		return std::nullopt;
	}

	int tileSize = renderer.get_tile_size();
	int panelW = LEFT_W_TILES * tileSize;
	int panelY = TOP_H_TILES * tileSize;
	int panelH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mousePx < 0 || mousePx >= panelW)
	{
		return std::nullopt;
	}
	if (mousePy < panelY || mousePy >= panelY + panelH)
	{
		return std::nullopt;
	}

	int maxVisible = panelH / tileSize;
	int slot = (mousePy - panelY) / tileSize;
	int palSize = static_cast<int>(library->ordered_palette().size());

	int scrollStart = 0;
	if (paletteIndex >= maxVisible)
	{
		scrollStart = paletteIndex - maxVisible + 1;
	}

	int idx = scrollStart + slot;
	if (idx < 0 || idx >= palSize || slot >= maxVisible)
	{
		return std::nullopt;
	}

	return idx;
}

// ---------------------------------------------------------------------------
// Symbol helpers
// ---------------------------------------------------------------------------

// Current selected symbol character.
char RoomEditor::selected_sym() const
{
	const auto& pal = library->ordered_palette();
	if (paletteIndex < 0 || paletteIndex >= static_cast<int>(pal.size()))
	{
		return '.';
	}
	return pal[paletteIndex].symbol;
}

// Tile to draw for a symbol in the canvas and palette.
TileRef RoomEditor::symbol_tile_id(char sym) const
{
	if (!tileConfig)
	{
		return TileRef{};
	}
	const auto& tileConfigRef = *tileConfig;
	switch (sym)
	{
	case '#':
		return tileConfigRef.get("TILE_WALL_STONE");
	case '.':
		return tileConfigRef.get("TILE_FLOOR_STONE");
	case ',':
		return tileConfigRef.get("TILE_CORRIDOR");
	case '+':
		return tileConfigRef.get("TILE_DOOR_CLOSED");
	case '~':
		return tileConfigRef.get("TILE_WATER");
	default:
		if (library)
		{
			return library->resolve_decor(sym);
		}
		return TileRef{};
	}
}

// 4-bit NESW bitmask: neighbour is '#' (or out-of-bounds) = wall.
int RoomEditor::canvas_wall_mask(int col, int row) const
{
	auto is_wall = [&](int wallCol, int wallRow)
	{
		if (wallRow < 0 || wallRow >= canvasHeight || wallCol < 0 || wallCol >= canvasWidth)
		{
			return false; // OOB = void, not wall -- prevents T-junctions on border
		}
		return canvas[wallRow][wallCol] == '#';
	};

	bool hasNorth = is_wall(col, row - 1);
	bool hasEast = is_wall(col + 1, row);
	bool hasSouth = is_wall(col, row + 1);
	bool hasWest = is_wall(col - 1, row);

	return (hasNorth ? 8 : 0) | (hasEast ? 4 : 0) | (hasSouth ? 2 : 0) | (hasWest ? 1 : 0);
}

// Position-aware tile that resolves wall/floor auto-tiling.
TileRef RoomEditor::canvas_tile_id(int col, int row) const
{
	char sym = canvas[row][col];

	if (sym == '#' && tileConfig)
	{
		return Autotile::wall_resolve_mask(tileConfig->get_wall_autotile("WALL_AUTOTILE_STONE"), canvas_wall_mask(col, row));
	}

	if (sym == '.' && tileConfig)
	{
		auto is_floor = [&](int floorCol, int floorRow) -> bool
		{
			if (floorRow < 0 || floorRow >= canvasHeight || floorCol < 0 || floorCol >= canvasWidth)
			{
				return false;
			}
			return canvas[floorRow][floorCol] == '.';
		};
		return Autotile::resolve(
			tileConfig->get_autotile("AUTOTILE_FLOOR_STONE"),
			is_floor(col, row - 1),
			is_floor(col + 1, row),
			is_floor(col, row + 1),
			is_floor(col - 1, row));
	}

	return symbol_tile_id(sym);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void RoomEditor::render(const GameContext& ctx) const
{
	const Renderer& renderer = *ctx.renderer;

	ClearBackground(Color{ 10, 10, 14, 255 });

	render_top_bar(renderer);
	render_left_panel(renderer);
	render_canvas(renderer);
	render_right_panel(renderer);
	render_bottom_bar(renderer);

	if (mode == EditorMode::TILE_PICKER)
	{
		render_tile_picker(renderer);
	}
	else if (mode != EditorMode::NORMAL)
	{
		render_input_overlay(renderer);
	}
}

void RoomEditor::render_top_bar(const Renderer& renderer) const
{
	int tileSize = renderer.get_tile_size();
	int screenWidth = renderer.get_screen_width();

	DrawRectangle(0, 0, screenWidth, tileSize, Color{ 20, 20, 30, 240 });
	DrawLine(0, tileSize - 1, screenWidth, tileSize - 1, Color{ 80, 80, 120, 255 });

	bool savedFlash = (GetTime() - statusTime) < 3.0;

	std::string title = std::format(
		"ROOM EDITOR  |  {}  |  {}x{}  |  Depth: {}-{}  |  Weight: {:.1f}{}",
		prefabName,
		canvasWidth,
		canvasHeight,
		depthMin,
		depthMax,
		weight,
		savedFlash ? std::format("  --  {}", statusMsg) : "");

	Color titleCol = savedFlash
		? Color{ 100, 255, 140, 255 }
		: Color{ 200, 200, 255, 255 };

	renderer.draw_text_color(Vector2D{ 8, (tileSize - renderer.get_font_size()) / 2 }, title, titleCol);
}

void RoomEditor::render_left_panel(const Renderer& renderer) const
{
	if (!library)
	{
		return;
	}

	int tileSize = renderer.get_tile_size();
	int panelW = LEFT_W_TILES * tileSize;
	int panelY = TOP_H_TILES * tileSize;
	int panelH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	DrawRectangle(0, panelY, panelW, panelH, Color{ 15, 15, 22, 230 });
	DrawLine(panelW - 1, panelY, panelW - 1, panelY + panelH, Color{ 70, 70, 110, 255 });

	const auto& pal = library->ordered_palette();
	int maxVisible = panelH / tileSize;
	int palSize = static_cast<int>(pal.size());

	// Scroll to keep selected entry visible
	int scrollStart = 0;
	if (paletteIndex >= maxVisible)
	{
		scrollStart = paletteIndex - maxVisible + 1;
	}

	for (int slot = 0; slot < maxVisible && (scrollStart + slot) < palSize; ++slot)
	{
		int idx = scrollStart + slot;
		int py = panelY + slot * tileSize;
		char sym = pal[idx].symbol;

		bool selected = (idx == paletteIndex);

		if (selected)
		{
			DrawRectangle(0, py, panelW, tileSize, Color{ 60, 60, 100, 200 });
		}

		TileRef tile = symbol_tile_id(sym);
		bool isStructural = (sym == '#' || sym == '.' || sym == ',' ||
			sym == '+' || sym == '~');

		if (isStructural)
		{
			if (tile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ 2, py }, tile);
			}
			else
			{
				Color block{};
				if (sym == '#')
				{
					block = Color{ 90, 90, 100, 255 };
				}
				else if (sym == '.')
				{
					block = Color{ 70, 55, 40, 255 };
				}
				else if (sym == ',')
				{
					block = Color{ 55, 55, 60, 255 };
				}
				else if (sym == '+')
				{
					block = Color{ 100, 70, 40, 255 };
				}
				else if (sym == '~')
				{
					block = Color{ 30, 60, 120, 255 };
				}
				else
				{
					block = Color{ 50, 50, 50, 255 };
				}
				DrawRectangle(4, py + 4, tileSize - 8, tileSize - 8, block);
			}
		}
		else
		{
			DrawRectangle(2, py, tileSize, tileSize, Color{ 70, 58, 42, 255 });
			if (tile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ 2, py }, tile);
			}
		}

		// Label
		Color labelCol = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 180, 180, 200, 200 };

		renderer.draw_text_color(Vector2D{ tileSize + 4, py + (tileSize - renderer.get_font_size()) / 2 }, pal[idx].label, labelCol);

		if (selected)
		{
			DrawRectangleLines(1, py + 1, panelW - 2, tileSize - 2, Color{ 200, 200, 100, 180 });
		}
	}
}

void RoomEditor::render_canvas(const Renderer& renderer) const
{
	int tileSize = renderer.get_tile_size();
	int areaX = LEFT_W_TILES * tileSize;
	int areaY = TOP_H_TILES * tileSize;
	int areaW = renderer.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * tileSize;
	int areaH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	// Dark canvas background
	DrawRectangle(areaX, areaY, areaW, areaH, Color{ 8, 8, 12, 255 });

	BeginScissorMode(areaX, areaY, areaW, areaH);

	int startCol = std::max(0, panX / tileSize);
	int startRow = std::max(0, panY / tileSize);
	int endCol = std::min(canvasWidth, startCol + areaW / tileSize + 2);
	int endRow = std::min(canvasHeight, startRow + areaH / tileSize + 2);

	static constexpr Color FLOOR_BG = { 70, 58, 42, 255 };
	static constexpr Color GRID_COLOR = { 80, 80, 110, 200 };

	for (int row = startRow; row < endRow; ++row)
	{
		for (int col = startCol; col < endCol; ++col)
		{
			int px = areaX + col * tileSize - panX;
			int py = areaY + row * tileSize - panY;

			// Base layer (structural)
			TileRef baseTile = canvas_tile_id(col, row);
			if (baseTile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ px, py }, baseTile);
			}
			else
			{
				DrawRectangle(px, py, tileSize, tileSize, FLOOR_BG);
			}

			// Decor layer -- additively blended so black sprite pixels
			// are transparent and the base tile shows through underneath.
			char decorSym = (row < static_cast<int>(decorCanvas.size()) &&
								 col < static_cast<int>(decorCanvas[row].size()))
				? decorCanvas[row][col]
				: ' ';

			if (decorSym != ' ')
			{
				TileRef decorTile = symbol_tile_id(decorSym);
				if (decorTile.is_valid())
				{
					renderer.draw_tile_screen(Vector2D{ px, py }, decorTile);
				}
			}

			DrawRectangleLines(px, py, tileSize, tileSize, GRID_COLOR);
		}
	}

	EndScissorMode();

	// Panel borders
	DrawLine(areaX, areaY, areaX, areaY + areaH, Color{ 70, 70, 110, 255 });
	DrawLine(areaX + areaW, areaY, areaX + areaW, areaY + areaH, Color{ 70, 70, 110, 255 });

	// Canvas size label in top-left corner of canvas area
	std::string dimLabel = std::format("{}x{}", canvasWidth, canvasHeight);
	renderer.draw_text_color(Vector2D{ areaX + 4, areaY + 4 }, dimLabel, Color{ 100, 100, 140, 160 });
}

void RoomEditor::render_right_panel(const Renderer& renderer) const
{
	if (!library)
	{
		return;
	}

	int tileSize = renderer.get_tile_size();
	int panelX = renderer.get_screen_width() - RIGHT_W_TILES * tileSize;
	int panelY = TOP_H_TILES * tileSize;
	int panelH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;
	int panelW = RIGHT_W_TILES * tileSize;

	DrawRectangle(panelX, panelY, panelW, panelH, Color{ 15, 15, 22, 230 });
	DrawLine(panelX, panelY, panelX, panelY + panelH, Color{ 70, 70, 110, 255 });

	// Header
	renderer.draw_text_color(Vector2D{ panelX + 6, panelY + 4 }, "SAVED PREFABS", Color{ 160, 160, 220, 255 });

	int fontH = renderer.get_font_size() + 4;
	int listYOff = fontH + 4;
	int listH = panelH - listYOff;
	int visible = listH / fontH;
	const auto& all = library->all();
	int count = static_cast<int>(all.size());

	for (int slot = 0; slot < visible && (listScroll + slot) < count; ++slot)
	{
		int idx = listScroll + slot;
		int py = panelY + listYOff + slot * fontH;

		bool selected = (idx == listSelected);

		if (selected)
		{
			DrawRectangle(panelX, py, panelW, fontH, Color{ 50, 50, 90, 200 });
		}

		const Prefab& p = all[idx];
		std::string label = std::format("{}  {}x{}", p.name, p.width(), p.height());

		Color entryCol = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 200, 200 };

		renderer.draw_text_color(Vector2D{ panelX + 6, py + 2 }, label, entryCol);
	}

	// Empty state
	if (count == 0)
	{
		renderer.draw_text_color(
			Vector2D{ panelX + 6, panelY + listYOff + 4 }, "No prefabs yet.", Color{ 100, 100, 120, 150 });
	}

	// Workflow guidance -- drawn in the bottom portion of the right panel
	int guideLines = 8;
	int guideH = guideLines * fontH + 8;
	int guideY = panelY + panelH - guideH;

	DrawLine(panelX, guideY, panelX + panelW, guideY, Color{ 60, 60, 100, 200 });
	DrawRectangle(panelX, guideY + 1, panelW, guideH - 1, Color{ 10, 10, 18, 200 });

	renderer.draw_text_color(Vector2D{ panelX + 6, guideY + 4 }, "HOW IT WORKS", Color{ 140, 140, 220, 255 });

	static constexpr const char* GUIDE[] = {
		"[N]      new canvas",
		"[Ctrl+S] name + save",
		"[Enter]  load selected",
		"[Del]    delete selected",
		"Prefabs stamp decor",
		"onto BSP rooms at",
		"matching depth + size."
	};

	Color guideCol{ 120, 120, 160, 200 };
	int guideTextY = guideY + fontH + 6;

	for (const char* line : GUIDE)
	{
		renderer.draw_text_color(Vector2D{ panelX + 6, guideTextY }, line, guideCol);
		guideTextY += fontH;
	}
}

void RoomEditor::render_bottom_bar(const Renderer& renderer) const
{
	int tileSize = renderer.get_tile_size();
	int screenWidth = renderer.get_screen_width();
	int barY = renderer.get_screen_height() - tileSize;

	DrawRectangle(0, barY, screenWidth, tileSize, Color{ 20, 20, 30, 240 });
	DrawLine(0, barY, screenWidth, barY, Color{ 80, 80, 120, 255 });

	std::string_view controls =
		"L=paint  R=erase  Mid=pan  Up/Dn=pal  +/-=zoom  F2=tile  F3=label  Ctrl+S=save  Esc=exit";

	renderer.draw_text_color(Vector2D{ 8, barY + (tileSize - renderer.get_font_size()) / 2 }, controls, Color{ 140, 140, 180, 220 });
}

void RoomEditor::render_input_overlay(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	// Semi-transparent backdrop
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 160 });

	std::string prompt;
	if (mode == EditorMode::INPUT_WIDTH)
	{
		prompt = std::format("New canvas width (current {}): {}_", canvasWidth, inputBuffer);
	}
	else if (mode == EditorMode::INPUT_HEIGHT)
	{
		prompt = std::format("New canvas height (current {}): {}_", canvasHeight, inputBuffer);
	}
	else if (mode == EditorMode::INPUT_NAME)
	{
		prompt = std::format("Prefab name: {}_", inputBuffer);
	}
	else if (mode == EditorMode::INPUT_LABEL)
	{
		prompt = std::format("Symbol label: {}_", inputBuffer);
	}

	int fontH = renderer.get_font_size();
	int textW = renderer.measure_text(prompt);
	int boxW = textW + 32;
	int boxH = fontH + 20;
	int boxX = (screenWidth - boxW) / 2;
	int boxY = (screenHeight - boxH) / 2;

	DrawRectangle(boxX, boxY, boxW, boxH, Color{ 20, 20, 35, 240 });
	DrawRectangleLines(boxX, boxY, boxW, boxH, Color{ 120, 120, 200, 255 });
	renderer.draw_text_color(Vector2D{ boxX + 16, boxY + 10 }, prompt, Color{ 220, 220, 255, 255 });
}

// ---------------------------------------------------------------------------
// Tile picker (F2)
// ---------------------------------------------------------------------------

// Sheets available in the picker, in Tab-cycle order.
static constexpr TileSheet PICKER_SHEETS[] = {
	TileSheet::SHEET_DECOR0, TileSheet::SHEET_DECOR1, TileSheet::SHEET_FLOOR, TileSheet::SHEET_WALL, TileSheet::SHEET_TILE
};
static constexpr int PICKER_SHEET_COUNT = 5;
static constexpr std::string_view PICKER_SHEET_NAMES[] = {
	"Decor0", "Decor1", "Floor", "Wall", "Tile"
};

void RoomEditor::handle_input_picker(GameContext& ctx)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		mode = EditorMode::NORMAL;
		return;
	}

	const Renderer& renderer = *ctx.renderer;
	TileSheet sheetId = PICKER_SHEETS[pickerSheetListIndex];
	int cols = std::max(1, renderer.get_sheet_cols(sheetId));
	int rows = std::max(1, renderer.get_sheet_rows(sheetId));

	if (IsKeyPressed(KEY_TAB))
	{
		int dir = (IsKeyDown(KEY_LEFT_SHIFT)) ? -1 : 1;
		pickerSheetListIndex = (pickerSheetListIndex + PICKER_SHEET_COUNT + dir) % PICKER_SHEET_COUNT;
		pickerCol = 0;
		pickerRow = 0;
		return;
	}

	if (IsKeyPressed(KEY_LEFT))
	{
		pickerCol = (pickerCol - 1 + cols) % cols;
	}
	if (IsKeyPressed(KEY_RIGHT))
	{
		pickerCol = (pickerCol + 1) % cols;
	}
	if (IsKeyPressed(KEY_UP))
	{
		pickerRow = (pickerRow - 1 + rows) % rows;
	}
	if (IsKeyPressed(KEY_DOWN))
	{
		pickerRow = (pickerRow + 1) % rows;
	}

	if (IsKeyPressed(KEY_ENTER) && library)
	{
		const auto& pal = library->ordered_palette();
		if (paletteIndex >= 0 && paletteIndex < static_cast<int>(pal.size()))
		{
			char sym = pal[paletteIndex].symbol;
			TileRef tile{ sheetId, pickerCol, pickerRow };
			library->set_symbol_tile(sym, tile);
			library->save(Paths::PREFABS);
			set_status(std::format("'{}' -> tile ({},{}) sheet {}", sym, pickerCol, pickerRow, PICKER_SHEET_NAMES[pickerSheetListIndex]));
		}
		mode = EditorMode::NORMAL;
	}
}

void RoomEditor::render_tile_picker(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	// Dark overlay over editor
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 200 });

	// Picker tile display size -- 2x native for visibility
	static constexpr int PICK_TS = SPRITE_SIZE * 2;

	TileSheet sheetId = PICKER_SHEETS[pickerSheetListIndex];
	int cols = std::max(1, renderer.get_sheet_cols(sheetId));
	int rows = std::max(1, renderer.get_sheet_rows(sheetId));

	// Center the grid
	int gridW = cols * PICK_TS;
	int gridH = rows * PICK_TS;
	int offX = (screenWidth - gridW) / 2;
	int offY = 48; // leave room for top info bar

	// Background for grid area
	DrawRectangle(offX - 2, offY - 2, gridW + 4, gridH + 4, Color{ 10, 10, 18, 255 });

	// Draw every tile in the sheet at PICK_TS size
	for (int row = 0; row < rows; ++row)
	{
		for (int col = 0; col < cols; ++col)
		{
			int px = offX + col * PICK_TS;
			int py = offY + row * PICK_TS;
			TileRef tileRef{ sheetId, col, row };

			// Dark bg so decor tiles are visible
			DrawRectangle(px, py, PICK_TS, PICK_TS, Color{ 20, 16, 12, 255 });

			renderer.draw_tile_screen_sized(Vector2D{ px, py }, tileRef, PICK_TS);

			// Selected cell highlight
			if (col == pickerCol && row == pickerRow)
			{
				DrawRectangleLines(px, py, PICK_TS, PICK_TS, Color{ 255, 230, 50, 255 });
			}
		}
	}

	// Info bar at top
	const auto* palPtr = library ? &library->ordered_palette() : nullptr;
	std::string symInfo = "?";
	if (palPtr && paletteIndex >= 0 &&
		paletteIndex < static_cast<int>(palPtr->size()))
	{
		char sym = (*palPtr)[paletteIndex].symbol;
		symInfo = std::format("Assigning tile for '{}' ({})",
			sym,
			library->symbol_label(sym));
	}

	std::string header = std::format(
		"TILE PICKER  |  {}  |  Sheet {}/{}  [Tab=next  Shift+Tab=prev]  "
		"|  col={} row={}  |  [Enter]=assign  [Esc]=cancel",
		symInfo,
		pickerSheetListIndex + 1,
		PICKER_SHEET_COUNT,
		pickerCol,
		pickerRow);

	DrawRectangle(0, 0, screenWidth, 40, Color{ 20, 20, 35, 240 });
	renderer.draw_text_color(Vector2D{ 8, 10 }, header, Color{ 220, 220, 255, 255 });

	// Sheet name label centered below grid
	int nameY = offY + gridH + 8;
	std::string sheetLabel = std::format("Sheet: {}  ({} cols x {} rows)",
		PICKER_SHEET_NAMES[pickerSheetListIndex],
		cols,
		rows);
	renderer.draw_text_color(Vector2D{ offX, nameY }, sheetLabel, Color{ 160, 160, 220, 200 });
}
