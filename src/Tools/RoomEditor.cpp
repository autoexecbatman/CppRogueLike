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

void RoomEditor::new_canvas(int w, int h)
{
	canvasWidth = std::max(3, w);
	canvasHeight = std::max(3, h);
	canvas.assign(canvasHeight, std::string(canvasWidth, '.'));
	decorCanvas.assign(canvasHeight, std::string(canvasWidth, ' '));

	// Fill border with walls
	for (int c = 0; c < canvasWidth; ++c)
	{
		canvas[0][c] = '#';
		canvas[canvasHeight - 1][c] = '#';
	}
	for (int r = 0; r < canvasHeight; ++r)
	{
		canvas[r][0] = '#';
		canvas[r][canvasWidth - 1] = '#';
	}

	panX = 0;
	panY = 0;
}

void RoomEditor::do_save(const std::string& name)
{
	if (!library)
		return;

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
		std::string row_str;
		row_str.reserve(canvasWidth);
		for (int col = 0; col < canvasWidth; ++col)
		{
			char d = (row < static_cast<int>(decorCanvas.size()) &&
						 col < static_cast<int>(decorCanvas[row].size()))
				? decorCanvas[row][col]
				: ' ';

			char b = (row < static_cast<int>(canvas.size()) &&
						 col < static_cast<int>(canvas[row].size()))
				? canvas[row][col]
				: '.';

			row_str += (d != ' ') ? d : b;
		}
		p.rows.push_back(std::move(row_str));
	}

	prefabName = p.name;
	library->add_or_replace(std::move(p));
	library->save(Paths::PREFABS);
	set_status(std::format("Saved: {}", prefabName));
}

void RoomEditor::do_load(int prefab_index)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefab_index < 0 || prefab_index >= static_cast<int>(all.size()))
	{
		return;
	}

	const Prefab& p = all[prefab_index];
	prefabName = p.name;
	depthMin = p.depthMin;
	depthMax = p.depthMax;
	weight = p.weight;

	canvasHeight = static_cast<int>(p.rows.size());
	canvasWidth = canvasHeight > 0 ? static_cast<int>(p.rows[0].size()) : 0;
	canvas.assign(canvasHeight, std::string(canvasWidth, '.'));
	decorCanvas.assign(canvasHeight, std::string(canvasWidth, ' '));

	auto is_structural_sym = [](char c) -> bool
	{
		return c == '#' || c == '.' || c == ',' || c == '+' || c == '~';
	};

	for (int row = 0; row < canvasHeight; ++row)
	{
		const std::string& src_row = p.rows[row];
		for (int col = 0; col < static_cast<int>(src_row.size()) && col < canvasWidth; ++col)
		{
			char sym = src_row[col];
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

void RoomEditor::do_delete(int prefab_index)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefab_index < 0 || prefab_index >= static_cast<int>(all.size()))
	{
		return;
	}

	std::string name = all[prefab_index].name;
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
	Renderer& r = *ctx.renderer;
	int tileSize = r.get_tile_size();

	// Escape -- exit
	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	// Global zoom
	if (IsKeyPressed(KEY_EQUAL))
	{
		r.zoom_in();
		return;
	}
	if (IsKeyPressed(KEY_MINUS))
	{
		r.zoom_out();
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
	int pal_size = static_cast<int>(pal.size());

	if (IsKeyPressed(KEY_UP))
		paletteIndex = (paletteIndex - 1 + pal_size) % pal_size;
	if (IsKeyPressed(KEY_DOWN))
		paletteIndex = (paletteIndex + 1) % pal_size;

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

		if (screen_to_palette_index(r, static_cast<int>(mw.x), static_cast<int>(mw.y)) >= 0 ||
			static_cast<int>(mw.x) < LEFT_W_TILES * tileSize)
		{
			int pal_size = static_cast<int>(library->ordered_palette().size());
			paletteIndex = std::clamp(paletteIndex + delta, 0, std::max(0, pal_size - 1));
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
		int pal_idx = screen_to_palette_index(r, static_cast<int>(mp.x), static_cast<int>(mp.y));
		if (pal_idx >= 0)
		{
			paletteIndex = pal_idx;
			return;
		}

		// Canvas
		if (screen_to_canvas(r, static_cast<int>(mp.x), static_cast<int>(mp.y), cx, cy))
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
		int list_idx = screen_to_list_index(r, static_cast<int>(mp.x), static_cast<int>(mp.y));
		if (list_idx >= 0)
		{
			listSelected = list_idx;
			return;
		}
	}

	// Right click -- erase on canvas
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		::Vector2 mp = GetMousePosition();
		int cx, cy;
		if (screen_to_canvas(r, static_cast<int>(mp.x), static_cast<int>(mp.y), cx, cy))
		{
			// First erase decor overlay; if already empty, erase the base tile.
			if (decorCanvas[cy][cx] != ' ')
			{
				decorCanvas[cy][cx] = ' ';
			}
			else
			{
				bool on_border = (cx == 0 || cx == canvasWidth - 1 || cy == 0 || cy == canvasHeight - 1);
				canvas[cy][cx] = on_border ? '#' : '.';
			}
		}
	}
}

void RoomEditor::handle_input_text(GameContext& ctx)
{
	// Backspace
	if (IsKeyPressed(KEY_BACKSPACE) && !inputBuffer.empty())
		inputBuffer.pop_back();

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
			int h = inputBuffer.empty() ? canvasHeight : std::stoi(inputBuffer);
			h = std::clamp(h, 3, 40);
			new_canvas(pendingWidth, h);
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
			accept = (ch >= '0' && ch <= '9') && inputBuffer.size() < 3;
		else
			accept = (ch >= 32 && ch <= 126) && inputBuffer.size() < 40;
		// INPUT_LABEL uses the same printable-character rule as INPUT_NAME

		if (accept)
			inputBuffer += static_cast<char>(ch);

		ch = GetCharPressed();
	}
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

// Returns canvas cell coords from screen pixel, false if outside canvas.
bool RoomEditor::screen_to_canvas(
	const Renderer& r,
	int mouse_px,
	int mouse_py,
	int& out_cx,
	int& out_cy) const
{
	int tileSize = r.get_tile_size();
	int area_x = LEFT_W_TILES * tileSize;
	int area_y = TOP_H_TILES * tileSize;
	int area_w = r.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * tileSize;
	int area_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mouse_px < area_x || mouse_px >= area_x + area_w)
		return false;
	if (mouse_py < area_y || mouse_py >= area_y + area_h)
		return false;

	int rel_x = mouse_px - area_x + panX;
	int rel_y = mouse_py - area_y + panY;

	out_cx = rel_x / tileSize;
	out_cy = rel_y / tileSize;

	if (out_cx < 0 || out_cx >= canvasWidth)
		return false;
	if (out_cy < 0 || out_cy >= canvasHeight)
		return false;

	return true;
}

// TODO: screen_to_list_index and screen_to_palette_index should return std::optional<int> -- -1 is a sentinel
// Returns prefab list index from screen pixel in right panel, -1 if none.
int RoomEditor::screen_to_list_index(const Renderer& r, int mouse_px, int mouse_py) const
{
	if (!library)
		return -1;

	int tileSize = r.get_tile_size();
	int panel_x = r.get_screen_width() - RIGHT_W_TILES * tileSize;
	int panel_y = TOP_H_TILES * tileSize;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mouse_px < panel_x || mouse_px >= r.get_screen_width())
		return -1;
	if (mouse_py < panel_y || mouse_py >= panel_y + panel_h)
		return -1;

	int font_h = r.get_font_size() + 2;
	int entry_count = static_cast<int>(library->all().size());
	int visible = panel_h / font_h;
	int row = (mouse_py - panel_y) / font_h;
	int idx = listScroll + row;

	if (idx < 0 || idx >= entry_count || row >= visible)
		return -1;

	return idx;
}

// Returns palette index from screen pixel in left panel, -1 if none.
int RoomEditor::screen_to_palette_index(const Renderer& r, int mouse_px, int mouse_py) const
{
	if (!library)
		return -1;

	int tileSize = r.get_tile_size();
	int panel_w = LEFT_W_TILES * tileSize;
	int panel_y = TOP_H_TILES * tileSize;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mouse_px < 0 || mouse_px >= panel_w)
		return -1;
	if (mouse_py < panel_y || mouse_py >= panel_y + panel_h)
		return -1;

	int max_visible = panel_h / tileSize;
	int slot = (mouse_py - panel_y) / tileSize;
	int pal_size = static_cast<int>(library->ordered_palette().size());

	int scroll_start = 0;
	if (paletteIndex >= max_visible)
		scroll_start = paletteIndex - max_visible + 1;

	int idx = scroll_start + slot;
	if (idx < 0 || idx >= pal_size || slot >= max_visible)
		return -1;

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
		return '.';
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
			return library->resolve_decor(sym);
		return TileRef{};
	}
}

// 4-bit NESW bitmask: neighbour is '#' (or out-of-bounds) = wall.
int RoomEditor::canvas_wall_mask(int col, int row) const
{
	auto is_wall = [&](int c, int r)
	{
		if (r < 0 || r >= canvasHeight || c < 0 || c >= canvasWidth)
			return false; // OOB = void, not wall -- prevents T-junctions on border
		return canvas[r][c] == '#';
	};

	bool n = is_wall(col, row - 1);
	bool e = is_wall(col + 1, row);
	bool s = is_wall(col, row + 1);
	bool w = is_wall(col - 1, row);

	return (n ? 8 : 0) | (e ? 4 : 0) | (s ? 2 : 0) | (w ? 1 : 0);
}

// Position-aware tile that resolves wall/floor auto-tiling.
TileRef RoomEditor::canvas_tile_id(int col, int row) const
{
	char sym = canvas[row][col];

	if (sym == '#' && tileConfig)
		return Autotile::wall_resolve_mask(tileConfig->get_wall_autotile("WALL_AUTOTILE_STONE"), canvas_wall_mask(col, row));

	if (sym == '.' && tileConfig)
	{
		auto is_floor = [&](int c, int r) -> bool
		{
			if (r < 0 || r >= canvasHeight || c < 0 || c >= canvasWidth)
				return false;
			return canvas[r][c] == '.';
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
	const Renderer& r = *ctx.renderer;

	ClearBackground(Color{ 10, 10, 14, 255 });

	render_top_bar(r);
	render_left_panel(r);
	render_canvas(r);
	render_right_panel(r);
	render_bottom_bar(r);

	if (mode == EditorMode::TILE_PICKER)
		render_tile_picker(r);
	else if (mode != EditorMode::NORMAL)
		render_input_overlay(r);
}

void RoomEditor::render_top_bar(const Renderer& r) const
{
	int tileSize = r.get_tile_size();
	int screenWidth = r.get_screen_width();

	DrawRectangle(0, 0, screenWidth, tileSize, Color{ 20, 20, 30, 240 });
	DrawLine(0, tileSize - 1, screenWidth, tileSize - 1, Color{ 80, 80, 120, 255 });

	bool saved_flash = (GetTime() - statusTime) < 3.0;

	std::string title = std::format(
		"ROOM EDITOR  |  {}  |  {}x{}  |  Depth: {}-{}  |  Weight: {:.1f}{}",
		prefabName,
		canvasWidth,
		canvasHeight,
		depthMin,
		depthMax,
		weight,
		saved_flash ? std::format("  --  {}", statusMsg) : "");

	Color title_col = saved_flash
		? Color{ 100, 255, 140, 255 }
		: Color{ 200, 200, 255, 255 };

	r.draw_text_color(Vector2D{ 8, (tileSize - r.get_font_size()) / 2 }, title, title_col);
}

void RoomEditor::render_left_panel(const Renderer& r) const
{
	if (!library)
		return;

	int tileSize = r.get_tile_size();
	int panel_w = LEFT_W_TILES * tileSize;
	int panel_y = TOP_H_TILES * tileSize;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	DrawRectangle(0, panel_y, panel_w, panel_h, Color{ 15, 15, 22, 230 });
	DrawLine(panel_w - 1, panel_y, panel_w - 1, panel_y + panel_h, Color{ 70, 70, 110, 255 });

	const auto& pal = library->ordered_palette();
	int max_visible = panel_h / tileSize;
	int pal_size = static_cast<int>(pal.size());

	// Scroll to keep selected entry visible
	int scroll_start = 0;
	if (paletteIndex >= max_visible)
		scroll_start = paletteIndex - max_visible + 1;

	for (int slot = 0; slot < max_visible && (scroll_start + slot) < pal_size; ++slot)
	{
		int idx = scroll_start + slot;
		int py = panel_y + slot * tileSize;
		char sym = pal[idx].symbol;

		bool selected = (idx == paletteIndex);

		if (selected)
			DrawRectangle(0, py, panel_w, tileSize, Color{ 60, 60, 100, 200 });

		TileRef tile = symbol_tile_id(sym);
		bool is_structural = (sym == '#' || sym == '.' || sym == ',' ||
			sym == '+' || sym == '~');

		if (is_structural)
		{
			if (tile.is_valid())
				r.draw_tile_screen(Vector2D{ 2, py }, tile);
			else
			{
				Color block{};
				if (sym == '#')
					block = Color{ 90, 90, 100, 255 };
				else if (sym == '.')
					block = Color{ 70, 55, 40, 255 };
				else if (sym == ',')
					block = Color{ 55, 55, 60, 255 };
				else if (sym == '+')
					block = Color{ 100, 70, 40, 255 };
				else if (sym == '~')
					block = Color{ 30, 60, 120, 255 };
				else
					block = Color{ 50, 50, 50, 255 };
				DrawRectangle(4, py + 4, tileSize - 8, tileSize - 8, block);
			}
		}
		else
		{
			DrawRectangle(2, py, tileSize, tileSize, Color{ 70, 58, 42, 255 });
			if (tile.is_valid())
				r.draw_tile_screen(Vector2D{ 2, py }, tile);
		}

		// Label
		Color label_col = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 180, 180, 200, 200 };

		r.draw_text_color(Vector2D{ tileSize + 4, py + (tileSize - r.get_font_size()) / 2 }, pal[idx].label, label_col);

		if (selected)
			DrawRectangleLines(1, py + 1, panel_w - 2, tileSize - 2, Color{ 200, 200, 100, 180 });
	}
}

void RoomEditor::render_canvas(const Renderer& r) const
{
	int tileSize = r.get_tile_size();
	int area_x = LEFT_W_TILES * tileSize;
	int area_y = TOP_H_TILES * tileSize;
	int area_w = r.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * tileSize;
	int area_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	// Dark canvas background
	DrawRectangle(area_x, area_y, area_w, area_h, Color{ 8, 8, 12, 255 });

	BeginScissorMode(area_x, area_y, area_w, area_h);

	int start_col = std::max(0, panX / tileSize);
	int start_row = std::max(0, panY / tileSize);
	int end_col = std::min(canvasWidth, start_col + area_w / tileSize + 2);
	int end_row = std::min(canvasHeight, start_row + area_h / tileSize + 2);

	static constexpr Color FLOOR_BG = { 70, 58, 42, 255 };
	static constexpr Color GRID_COLOR = { 80, 80, 110, 200 };

	for (int row = start_row; row < end_row; ++row)
	{
		for (int col = start_col; col < end_col; ++col)
		{
			int px = area_x + col * tileSize - panX;
			int py = area_y + row * tileSize - panY;

			// Base layer (structural)
			TileRef base_tile = canvas_tile_id(col, row);
			if (base_tile.is_valid())
				r.draw_tile_screen(Vector2D{ px, py }, base_tile);
			else
				DrawRectangle(px, py, tileSize, tileSize, FLOOR_BG);

			// Decor layer -- additively blended so black sprite pixels
			// are transparent and the base tile shows through underneath.
			char decor_sym = (row < static_cast<int>(decorCanvas.size()) &&
								 col < static_cast<int>(decorCanvas[row].size()))
				? decorCanvas[row][col]
				: ' ';

			if (decor_sym != ' ')
			{
				TileRef decor_tile = symbol_tile_id(decor_sym);
				if (decor_tile.is_valid())
					r.draw_tile_screen(Vector2D{ px, py }, decor_tile);
			}

			DrawRectangleLines(px, py, tileSize, tileSize, GRID_COLOR);
		}
	}

	EndScissorMode();

	// Panel borders
	DrawLine(area_x, area_y, area_x, area_y + area_h, Color{ 70, 70, 110, 255 });
	DrawLine(area_x + area_w, area_y, area_x + area_w, area_y + area_h, Color{ 70, 70, 110, 255 });

	// Canvas size label in top-left corner of canvas area
	std::string dim_label = std::format("{}x{}", canvasWidth, canvasHeight);
	r.draw_text_color(Vector2D{ area_x + 4, area_y + 4 }, dim_label, Color{ 100, 100, 140, 160 });
}

void RoomEditor::render_right_panel(const Renderer& r) const
{
	if (!library)
		return;

	int tileSize = r.get_tile_size();
	int panel_x = r.get_screen_width() - RIGHT_W_TILES * tileSize;
	int panel_y = TOP_H_TILES * tileSize;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;
	int panel_w = RIGHT_W_TILES * tileSize;

	DrawRectangle(panel_x, panel_y, panel_w, panel_h, Color{ 15, 15, 22, 230 });
	DrawLine(panel_x, panel_y, panel_x, panel_y + panel_h, Color{ 70, 70, 110, 255 });

	// Header
	r.draw_text_color(Vector2D{ panel_x + 6, panel_y + 4 }, "SAVED PREFABS", Color{ 160, 160, 220, 255 });

	int font_h = r.get_font_size() + 4;
	int list_y_off = font_h + 4;
	int list_h = panel_h - list_y_off;
	int visible = list_h / font_h;
	const auto& all = library->all();
	int count = static_cast<int>(all.size());

	for (int slot = 0; slot < visible && (listScroll + slot) < count; ++slot)
	{
		int idx = listScroll + slot;
		int py = panel_y + list_y_off + slot * font_h;

		bool selected = (idx == listSelected);

		if (selected)
			DrawRectangle(panel_x, py, panel_w, font_h, Color{ 50, 50, 90, 200 });

		const Prefab& p = all[idx];
		std::string label = std::format("{}  {}x{}", p.name, p.width(), p.height());

		Color entry_col = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 200, 200 };

		r.draw_text_color(Vector2D{ panel_x + 6, py + 2 }, label, entry_col);
	}

	// Empty state
	if (count == 0)
	{
		r.draw_text_color(
			Vector2D{ panel_x + 6, panel_y + list_y_off + 4 }, "No prefabs yet.", Color{ 100, 100, 120, 150 });
	}

	// Workflow guidance -- drawn in the bottom portion of the right panel
	int guide_lines = 8;
	int guide_h = guide_lines * font_h + 8;
	int guide_y = panel_y + panel_h - guide_h;

	DrawLine(panel_x, guide_y, panel_x + panel_w, guide_y, Color{ 60, 60, 100, 200 });
	DrawRectangle(panel_x, guide_y + 1, panel_w, guide_h - 1, Color{ 10, 10, 18, 200 });

	r.draw_text_color(Vector2D{ panel_x + 6, guide_y + 4 }, "HOW IT WORKS", Color{ 140, 140, 220, 255 });

	static constexpr const char* GUIDE[] = {
		"[N]      new canvas",
		"[Ctrl+S] name + save",
		"[Enter]  load selected",
		"[Del]    delete selected",
		"Prefabs stamp decor",
		"onto BSP rooms at",
		"matching depth + size."
	};

	Color guide_col{ 120, 120, 160, 200 };
	int guide_text_y = guide_y + font_h + 6;

	for (const char* line : GUIDE)
	{
		r.draw_text_color(Vector2D{ panel_x + 6, guide_text_y }, line, guide_col);
		guide_text_y += font_h;
	}
}

void RoomEditor::render_bottom_bar(const Renderer& r) const
{
	int tileSize = r.get_tile_size();
	int screenWidth = r.get_screen_width();
	int bar_y = r.get_screen_height() - tileSize;

	DrawRectangle(0, bar_y, screenWidth, tileSize, Color{ 20, 20, 30, 240 });
	DrawLine(0, bar_y, screenWidth, bar_y, Color{ 80, 80, 120, 255 });

	std::string_view controls =
		"L=paint  R=erase  Mid=pan  Up/Dn=pal  +/-=zoom  F2=tile  F3=label  Ctrl+S=save  Esc=exit";

	r.draw_text_color(Vector2D{ 8, bar_y + (tileSize - r.get_font_size()) / 2 }, controls, Color{ 140, 140, 180, 220 });
}

void RoomEditor::render_input_overlay(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();

	// Semi-transparent backdrop
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 160 });

	std::string prompt;
	if (mode == EditorMode::INPUT_WIDTH)
		prompt = std::format("New canvas width (current {}): {}_", canvasWidth, inputBuffer);
	else if (mode == EditorMode::INPUT_HEIGHT)
		prompt = std::format("New canvas height (current {}): {}_", canvasHeight, inputBuffer);
	else if (mode == EditorMode::INPUT_NAME)
		prompt = std::format("Prefab name: {}_", inputBuffer);
	else if (mode == EditorMode::INPUT_LABEL)
		prompt = std::format("Symbol label: {}_", inputBuffer);

	int font_h = r.get_font_size();
	int text_w = r.measure_text(prompt);
	int box_w = text_w + 32;
	int box_h = font_h + 20;
	int box_x = (screenWidth - box_w) / 2;
	int box_y = (screenHeight - box_h) / 2;

	DrawRectangle(box_x, box_y, box_w, box_h, Color{ 20, 20, 35, 240 });
	DrawRectangleLines(box_x, box_y, box_w, box_h, Color{ 120, 120, 200, 255 });
	r.draw_text_color(Vector2D{ box_x + 16, box_y + 10 }, prompt, Color{ 220, 220, 255, 255 });
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

	const Renderer& r = *ctx.renderer;
	TileSheet sheet_id = PICKER_SHEETS[pickerSheetListIndex];
	int cols = std::max(1, r.get_sheet_cols(sheet_id));
	int rows = std::max(1, r.get_sheet_rows(sheet_id));

	if (IsKeyPressed(KEY_TAB))
	{
		int dir = (IsKeyDown(KEY_LEFT_SHIFT)) ? -1 : 1;
		pickerSheetListIndex = (pickerSheetListIndex + PICKER_SHEET_COUNT + dir) % PICKER_SHEET_COUNT;
		pickerCol = 0;
		pickerRow = 0;
		return;
	}

	if (IsKeyPressed(KEY_LEFT))
		pickerCol = (pickerCol - 1 + cols) % cols;
	if (IsKeyPressed(KEY_RIGHT))
		pickerCol = (pickerCol + 1) % cols;
	if (IsKeyPressed(KEY_UP))
		pickerRow = (pickerRow - 1 + rows) % rows;
	if (IsKeyPressed(KEY_DOWN))
		pickerRow = (pickerRow + 1) % rows;

	if (IsKeyPressed(KEY_ENTER) && library)
	{
		const auto& pal = library->ordered_palette();
		if (paletteIndex >= 0 && paletteIndex < static_cast<int>(pal.size()))
		{
			char sym = pal[paletteIndex].symbol;
			TileRef tile{ sheet_id, pickerCol, pickerRow };
			library->set_symbol_tile(sym, tile);
			library->save(Paths::PREFABS);
			set_status(std::format("'{}' -> tile ({},{}) sheet {}", sym, pickerCol, pickerRow, PICKER_SHEET_NAMES[pickerSheetListIndex]));
		}
		mode = EditorMode::NORMAL;
	}
}

void RoomEditor::render_tile_picker(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();

	// Dark overlay over editor
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 200 });

	// Picker tile display size -- 2x native for visibility
	static constexpr int PICK_TS = SPRITE_SIZE * 2;

	TileSheet sheet_id = PICKER_SHEETS[pickerSheetListIndex];
	int cols = std::max(1, r.get_sheet_cols(sheet_id));
	int rows = std::max(1, r.get_sheet_rows(sheet_id));

	// Center the grid
	int grid_w = cols * PICK_TS;
	int grid_h = rows * PICK_TS;
	int off_x = (screenWidth - grid_w) / 2;
	int off_y = 48; // leave room for top info bar

	// Background for grid area
	DrawRectangle(off_x - 2, off_y - 2, grid_w + 4, grid_h + 4, Color{ 10, 10, 18, 255 });

	// Draw every tile in the sheet at PICK_TS size
	for (int row = 0; row < rows; ++row)
	{
		for (int col = 0; col < cols; ++col)
		{
			int px = off_x + col * PICK_TS;
			int py = off_y + row * PICK_TS;
			TileRef tile_ref{ sheet_id, col, row };

			// Dark bg so decor tiles are visible
			DrawRectangle(px, py, PICK_TS, PICK_TS, Color{ 20, 16, 12, 255 });

			r.draw_tile_screen_sized(Vector2D{ px, py }, tile_ref, PICK_TS);

			// Selected cell highlight
			if (col == pickerCol && row == pickerRow)
				DrawRectangleLines(px, py, PICK_TS, PICK_TS, Color{ 255, 230, 50, 255 });
		}
	}

	// Info bar at top
	const auto* pal_ptr = library ? &library->ordered_palette() : nullptr;
	std::string sym_info = "?";
	if (pal_ptr && paletteIndex >= 0 &&
		paletteIndex < static_cast<int>(pal_ptr->size()))
	{
		char sym = (*pal_ptr)[paletteIndex].symbol;
		sym_info = std::format("Assigning tile for '{}' ({})",
			sym,
			library->symbol_label(sym));
	}

	std::string header = std::format(
		"TILE PICKER  |  {}  |  Sheet {}/{}  [Tab=next  Shift+Tab=prev]  "
		"|  col={} row={}  |  [Enter]=assign  [Esc]=cancel",
		sym_info,
		pickerSheetListIndex + 1,
		PICKER_SHEET_COUNT,
		pickerCol,
		pickerRow);

	DrawRectangle(0, 0, screenWidth, 40, Color{ 20, 20, 35, 240 });
	r.draw_text_color(Vector2D{ 8, 10 }, header, Color{ 220, 220, 255, 255 });

	// Sheet name label centered below grid
	int name_y = off_y + grid_h + 8;
	std::string sheet_label = std::format("Sheet: {}  ({} cols x {} rows)",
		PICKER_SHEET_NAMES[pickerSheetListIndex],
		cols,
		rows);
	r.draw_text_color(Vector2D{ off_x, name_y }, sheet_label, Color{ 160, 160, 220, 200 });
}
