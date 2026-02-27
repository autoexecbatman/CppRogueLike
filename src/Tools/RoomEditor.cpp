// file: RoomEditor.cpp
#include <algorithm>
#include <format>

#pragma warning(push, 0)
#include <raylib.h>
#pragma warning(pop)

// Undefine raylib color macros that clash with game enums
#undef YELLOW
#undef WHITE
#undef BLACK
#undef GRAY
#undef DARKGRAY
#undef GREEN
#undef RED

#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Menu/Menu.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/TileId.h"
#include "PrefabLibrary.h"
#include "RoomEditor.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RoomEditor::enter(PrefabLibrary& lib)
{
	library = &lib;
	active = true;
	mode = EditorMode::NORMAL;
	input_buf.clear();
	list_selected = -1;
	list_scroll = 0;
	status_msg.clear();
	new_canvas(canvas_w, canvas_h);
}

void RoomEditor::exit(GameContext& ctx)
{
	active = false;
	// Return to main menu
	ctx.menus->push_back(std::make_unique<Menu>(true, ctx));
}

void RoomEditor::tick(GameContext& ctx)
{
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
	canvas_w = std::max(3, w);
	canvas_h = std::max(3, h);
	canvas.assign(canvas_h, std::string(canvas_w, '.'));
	decor_canvas.assign(canvas_h, std::string(canvas_w, ' '));

	// Fill border with walls
	for (int c = 0; c < canvas_w; ++c)
	{
		canvas[0][c] = '#';
		canvas[canvas_h - 1][c] = '#';
	}
	for (int r = 0; r < canvas_h; ++r)
	{
		canvas[r][0] = '#';
		canvas[r][canvas_w - 1] = '#';
	}

	pan_x = 0;
	pan_y = 0;
}

void RoomEditor::do_save(const std::string& name)
{
	if (!library)
		return;

	Prefab p;
	p.name = name.empty() ? std::format("prefab_{}x{}", canvas_w, canvas_h) : name;
	p.depth_min = depth_min;
	p.depth_max = depth_max;
	p.weight = weight;

	// Merge base + decor layers: decor symbol wins when present.
	p.rows.clear();
	p.rows.reserve(canvas_h);
	for (int row = 0; row < canvas_h; ++row)
	{
		std::string row_str;
		row_str.reserve(canvas_w);
		for (int col = 0; col < canvas_w; ++col)
		{
			char d = (row < static_cast<int>(decor_canvas.size()) &&
						 col < static_cast<int>(decor_canvas[row].size()))
				? decor_canvas[row][col]
				: ' ';

			char b = (row < static_cast<int>(canvas.size()) &&
						 col < static_cast<int>(canvas[row].size()))
				? canvas[row][col]
				: '.';

			row_str += (d != ' ') ? d : b;
		}
		p.rows.push_back(std::move(row_str));
	}

	prefab_name = p.name;
	library->add_or_replace(std::move(p));
	library->save(Paths::PREFABS);
	set_status(std::format("Saved: {}", prefab_name));
}

void RoomEditor::do_load(int prefab_index)
{
	if (!library)
		return;
	const auto& all = library->all();
	if (prefab_index < 0 || prefab_index >= static_cast<int>(all.size()))
		return;

	const Prefab& p = all[prefab_index];
	prefab_name = p.name;
	depth_min = p.depth_min;
	depth_max = p.depth_max;
	weight = p.weight;

	canvas_h = static_cast<int>(p.rows.size());
	canvas_w = canvas_h > 0 ? static_cast<int>(p.rows[0].size()) : 0;
	canvas.assign(canvas_h, std::string(canvas_w, '.'));
	decor_canvas.assign(canvas_h, std::string(canvas_w, ' '));

	auto is_structural_sym = [](char c) -> bool
	{
		return c == '#' || c == '.' || c == ',' || c == '+' || c == '~';
	};

	for (int row = 0; row < canvas_h; ++row)
	{
		const std::string& src_row = p.rows[row];
		for (int col = 0; col < static_cast<int>(src_row.size()) && col < canvas_w; ++col)
		{
			char sym = src_row[col];
			if (is_structural_sym(sym))
			{
				canvas[row][col] = sym;
				decor_canvas[row][col] = ' ';
			}
			else
			{
				canvas[row][col] = '.';
				decor_canvas[row][col] = sym;
			}
		}
	}

	pan_x = 0;
	pan_y = 0;
	set_status(std::format("Loaded: {}", prefab_name));
}

void RoomEditor::do_delete(int prefab_index)
{
	if (!library)
		return;
	const auto& all = library->all();
	if (prefab_index < 0 || prefab_index >= static_cast<int>(all.size()))
		return;

	std::string name = all[prefab_index].name;
	library->remove(name);
	library->save(Paths::PREFABS);
	list_selected = -1;
	set_status(std::format("Deleted: {}", name));
}

void RoomEditor::set_status(const std::string& msg)
{
	status_msg = msg;
	status_time = GetTime();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void RoomEditor::handle_input(GameContext& ctx)
{
	if (mode == EditorMode::TILE_PICKER)
		handle_input_picker(ctx);
	else if (mode == EditorMode::NORMAL)
		handle_input_normal(ctx);
	else
		handle_input_text(ctx);
}

void RoomEditor::handle_input_normal(GameContext& ctx)
{
	Renderer& r = *ctx.renderer;
	int ts = r.get_tile_size();

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
		picker_sheet_list_idx = 0;
		picker_col = 0;
		picker_row = 0;
		mode = EditorMode::TILE_PICKER;
		return;
	}

	// F3 -- rename label for current palette symbol
	if (IsKeyPressed(KEY_F3) && library)
	{
		const auto& pal = library->ordered_palette();
		if (palette_index >= 0 && palette_index < static_cast<int>(pal.size()))
		{
			input_buf = pal[palette_index].second;
			mode = EditorMode::INPUT_LABEL;
		}
		return;
	}

	// Palette navigation
	const auto& pal = library->ordered_palette();
	int pal_size = static_cast<int>(pal.size());

	if (IsKeyPressed(KEY_UP))
		palette_index = (palette_index - 1 + pal_size) % pal_size;
	if (IsKeyPressed(KEY_DOWN))
		palette_index = (palette_index + 1) % pal_size;

	// New canvas
	if (IsKeyPressed(KEY_N))
	{
		mode = EditorMode::INPUT_WIDTH;
		input_buf.clear();
		return;
	}

	// Delete selected prefab
	if (IsKeyPressed(KEY_DELETE) && list_selected >= 0)
	{
		do_delete(list_selected);
		return;
	}

	// Load selected prefab
	if (IsKeyPressed(KEY_ENTER) && list_selected >= 0)
	{
		do_load(list_selected);
		return;
	}

	// Ctrl+S -- initiate save (enter name)
	if (IsKeyPressed(KEY_S) &&
		(IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
	{
		mode = EditorMode::INPUT_NAME;
		input_buf = prefab_name;
		return;
	}

	// Depth range (Ctrl+Up / Ctrl+Down adjusts depth_min/max)
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	if (ctrl && IsKeyPressed(KEY_UP))
	{
		depth_min = std::max(1, depth_min - 1);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_DOWN))
	{
		depth_min = std::min(depth_min + 1, depth_max);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_RIGHT))
	{
		depth_max = std::min(99, depth_max + 1);
		return;
	}
	if (ctrl && IsKeyPressed(KEY_LEFT))
	{
		depth_max = std::max(depth_min, depth_max - 1);
		return;
	}

	// Middle mouse -- pan
	if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
	{
		if (!panning)
		{
			panning = true;
			::Vector2 mp = GetMousePosition();
			pan_mouse_start_x = mp.x;
			pan_mouse_start_y = mp.y;
			pan_start_x = pan_x;
			pan_start_y = pan_y;
		}
		else
		{
			::Vector2 mp = GetMousePosition();
			pan_x = pan_start_x - static_cast<int>(mp.x - pan_mouse_start_x);
			pan_y = pan_start_y - static_cast<int>(mp.y - pan_mouse_start_y);
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
			static_cast<int>(mw.x) < LEFT_W_TILES * ts)
		{
			int pal_size = static_cast<int>(library->ordered_palette().size());
			palette_index = std::clamp(palette_index + delta, 0, std::max(0, pal_size - 1));
		}
		else
		{
			int count = static_cast<int>(library->all().size());
			list_scroll = std::clamp(list_scroll + delta, 0, std::max(0, count - 1));
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
			palette_index = pal_idx;
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
				decor_canvas[cy][cx] = ' '; // clear any decor overlay
			}
			else
			{
				decor_canvas[cy][cx] = sym; // layer over the base tile
			}
			return;
		}

		// Right panel list
		int list_idx = screen_to_list_index(r, static_cast<int>(mp.x), static_cast<int>(mp.y));
		if (list_idx >= 0)
		{
			list_selected = list_idx;
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
			if (decor_canvas[cy][cx] != ' ')
			{
				decor_canvas[cy][cx] = ' ';
			}
			else
			{
				bool on_border = (cx == 0 || cx == canvas_w - 1 || cy == 0 || cy == canvas_h - 1);
				canvas[cy][cx] = on_border ? '#' : '.';
			}
		}
	}
}

void RoomEditor::handle_input_text(GameContext& ctx)
{
	// Backspace
	if (IsKeyPressed(KEY_BACKSPACE) && !input_buf.empty())
		input_buf.pop_back();

	// Escape -- cancel
	if (IsKeyPressed(KEY_ESCAPE))
	{
		mode = EditorMode::NORMAL;
		input_buf.clear();
		return;
	}

	// Enter -- confirm
	if (IsKeyPressed(KEY_ENTER))
	{
		if (mode == EditorMode::INPUT_WIDTH)
		{
			pending_w = input_buf.empty() ? canvas_w : std::stoi(input_buf);
			pending_w = std::clamp(pending_w, 3, 60);
			mode = EditorMode::INPUT_HEIGHT;
			input_buf.clear();
		}
		else if (mode == EditorMode::INPUT_HEIGHT)
		{
			int h = input_buf.empty() ? canvas_h : std::stoi(input_buf);
			h = std::clamp(h, 3, 40);
			new_canvas(pending_w, h);
			mode = EditorMode::NORMAL;
			input_buf.clear();
		}
		else if (mode == EditorMode::INPUT_NAME)
		{
			do_save(input_buf);
			mode = EditorMode::NORMAL;
			input_buf.clear();
		}
		else if (mode == EditorMode::INPUT_LABEL)
		{
			if (library && !input_buf.empty())
			{
				const auto& pal = library->ordered_palette();
				if (palette_index >= 0 && palette_index < static_cast<int>(pal.size()))
				{
					char sym = pal[palette_index].first;
					library->set_symbol_label(sym, input_buf);
					library->save(Paths::PREFABS);
					set_status(std::format("Label: '{}'={}", sym, input_buf));
				}
			}
			mode = EditorMode::NORMAL;
			input_buf.clear();
		}
		return;
	}

	// Character input (digits for size modes, any printable for name)
	int ch = GetCharPressed();
	while (ch > 0)
	{
		bool accept = false;
		if (mode == EditorMode::INPUT_WIDTH || mode == EditorMode::INPUT_HEIGHT)
			accept = (ch >= '0' && ch <= '9') && input_buf.size() < 3;
		else
			accept = (ch >= 32 && ch <= 126) && input_buf.size() < 40;
		// INPUT_LABEL uses the same printable-character rule as INPUT_NAME

		if (accept)
			input_buf += static_cast<char>(ch);

		ch = GetCharPressed();
	}
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

bool RoomEditor::screen_to_canvas(
	const Renderer& r,
	int mouse_px,
	int mouse_py,
	int& out_cx,
	int& out_cy) const
{
	int ts = r.get_tile_size();
	int area_x = LEFT_W_TILES * ts;
	int area_y = TOP_H_TILES * ts;
	int area_w = r.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * ts;
	int area_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;

	if (mouse_px < area_x || mouse_px >= area_x + area_w)
		return false;
	if (mouse_py < area_y || mouse_py >= area_y + area_h)
		return false;

	int rel_x = mouse_px - area_x + pan_x;
	int rel_y = mouse_py - area_y + pan_y;

	out_cx = rel_x / ts;
	out_cy = rel_y / ts;

	if (out_cx < 0 || out_cx >= canvas_w)
		return false;
	if (out_cy < 0 || out_cy >= canvas_h)
		return false;

	return true;
}

int RoomEditor::screen_to_list_index(const Renderer& r, int mouse_px, int mouse_py) const
{
	if (!library)
		return -1;

	int ts = r.get_tile_size();
	int panel_x = r.get_screen_width() - RIGHT_W_TILES * ts;
	int panel_y = TOP_H_TILES * ts;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;

	if (mouse_px < panel_x || mouse_px >= r.get_screen_width())
		return -1;
	if (mouse_py < panel_y || mouse_py >= panel_y + panel_h)
		return -1;

	int font_h = r.get_font_size() + 2;
	int entry_count = static_cast<int>(library->all().size());
	int visible = panel_h / font_h;
	int row = (mouse_py - panel_y) / font_h;
	int idx = list_scroll + row;

	if (idx < 0 || idx >= entry_count || row >= visible)
		return -1;

	return idx;
}

int RoomEditor::screen_to_palette_index(const Renderer& r, int mouse_px, int mouse_py) const
{
	if (!library)
		return -1;

	int ts = r.get_tile_size();
	int panel_w = LEFT_W_TILES * ts;
	int panel_y = TOP_H_TILES * ts;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;

	if (mouse_px < 0 || mouse_px >= panel_w)
		return -1;
	if (mouse_py < panel_y || mouse_py >= panel_y + panel_h)
		return -1;

	int max_visible = panel_h / ts;
	int slot = (mouse_py - panel_y) / ts;
	int pal_size = static_cast<int>(library->ordered_palette().size());

	int scroll_start = 0;
	if (palette_index >= max_visible)
		scroll_start = palette_index - max_visible + 1;

	int idx = scroll_start + slot;
	if (idx < 0 || idx >= pal_size || slot >= max_visible)
		return -1;

	return idx;
}

// ---------------------------------------------------------------------------
// Symbol helpers
// ---------------------------------------------------------------------------

char RoomEditor::selected_sym() const
{
	const auto& pal = library->ordered_palette();
	if (palette_index < 0 || palette_index >= static_cast<int>(pal.size()))
		return '.';
	return pal[palette_index].first;
}

int RoomEditor::symbol_tile_id(char sym) const
{
	switch (sym)
	{
	case '#':
		return TILE_WALL_STONE;
	case '.':
		return TILE_FLOOR_STONE;
	case ',':
		return TILE_CORRIDOR;
	case '+':
		return TILE_DOOR_CLOSED;
	case '~':
		return TILE_WATER;
	default:
		if (library)
			return library->resolve_decor(sym);
		return 0;
	}
}

int RoomEditor::canvas_wall_mask(int col, int row) const
{
	auto is_wall = [&](int c, int r)
	{
		if (r < 0 || r >= canvas_h || c < 0 || c >= canvas_w)
			return false; // OOB = void, not wall -- prevents T-junctions on border
		return canvas[r][c] == '#';
	};

	bool n = is_wall(col, row - 1);
	bool e = is_wall(col + 1, row);
	bool s = is_wall(col, row + 1);
	bool w = is_wall(col - 1, row);

	return (n ? 8 : 0) | (e ? 4 : 0) | (s ? 2 : 0) | (w ? 1 : 0);
}

int RoomEditor::canvas_tile_id(int col, int row) const
{
	char sym = canvas[row][col];

	if (sym == '#')
		return wall_autotile_resolve_mask(WALL_AUTOTILE_STONE, canvas_wall_mask(col, row));

	if (sym == '.')
	{
		auto is_floor = [&](int c, int r) -> bool
		{
			if (r < 0 || r >= canvas_h || c < 0 || c >= canvas_w)
				return false;
			return canvas[r][c] == '.';
		};
		return autotile_resolve(
			AUTOTILE_FLOOR_STONE,
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
	int ts = r.get_tile_size();
	int sw = r.get_screen_width();

	DrawRectangle(0, 0, sw, ts, Color{ 20, 20, 30, 240 });
	DrawLine(0, ts - 1, sw, ts - 1, Color{ 80, 80, 120, 255 });

	bool saved_flash = (GetTime() - status_time) < 3.0;

	std::string title = std::format(
		"ROOM EDITOR  |  {}  |  {}x{}  |  Depth: {}-{}  |  Weight: {:.1f}{}",
		prefab_name,
		canvas_w,
		canvas_h,
		depth_min,
		depth_max,
		weight,
		saved_flash ? std::format("  --  {}", status_msg) : "");

	Color title_col = saved_flash
		? Color{ 100, 255, 140, 255 }
		: Color{ 200, 200, 255, 255 };

	r.draw_text_color(8, (ts - r.get_font_size()) / 2, title, title_col);
}

void RoomEditor::render_left_panel(const Renderer& r) const
{
	if (!library)
		return;

	int ts = r.get_tile_size();
	int panel_w = LEFT_W_TILES * ts;
	int panel_y = TOP_H_TILES * ts;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;

	DrawRectangle(0, panel_y, panel_w, panel_h, Color{ 15, 15, 22, 230 });
	DrawLine(panel_w - 1, panel_y, panel_w - 1, panel_y + panel_h, Color{ 70, 70, 110, 255 });

	const auto& pal = library->ordered_palette();
	int max_visible = panel_h / ts;
	int pal_size = static_cast<int>(pal.size());

	// Scroll to keep selected entry visible
	int scroll_start = 0;
	if (palette_index >= max_visible)
		scroll_start = palette_index - max_visible + 1;

	for (int slot = 0; slot < max_visible && (scroll_start + slot) < pal_size; ++slot)
	{
		int idx = scroll_start + slot;
		int py = panel_y + slot * ts;
		char sym = pal[idx].first;

		bool selected = (idx == palette_index);

		if (selected)
			DrawRectangle(0, py, panel_w, ts, Color{ 60, 60, 100, 200 });

		int tile_id = symbol_tile_id(sym);
		bool is_structural = (sym == '#' || sym == '.' || sym == ',' ||
			sym == '+' || sym == '~');

		if (is_structural)
		{
			if (tile_id != 0)
				r.draw_tile_screen(2, py, tile_id);
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
				DrawRectangle(4, py + 4, ts - 8, ts - 8, block);
			}
		}
		else
		{
			DrawRectangle(2, py, ts, ts, Color{ 70, 58, 42, 255 });
			if (tile_id != 0)
				r.draw_tile_screen(2, py, tile_id);
		}

		// Label
		Color label_col = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 180, 180, 200, 200 };

		r.draw_text_color(ts + 4, py + (ts - r.get_font_size()) / 2, pal[idx].second, label_col);

		if (selected)
			DrawRectangleLines(1, py + 1, panel_w - 2, ts - 2, Color{ 200, 200, 100, 180 });
	}
}

void RoomEditor::render_canvas(const Renderer& r) const
{
	int ts = r.get_tile_size();
	int area_x = LEFT_W_TILES * ts;
	int area_y = TOP_H_TILES * ts;
	int area_w = r.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * ts;
	int area_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;

	// Dark canvas background
	DrawRectangle(area_x, area_y, area_w, area_h, Color{ 8, 8, 12, 255 });

	BeginScissorMode(area_x, area_y, area_w, area_h);

	int start_col = std::max(0, pan_x / ts);
	int start_row = std::max(0, pan_y / ts);
	int end_col = std::min(canvas_w, start_col + area_w / ts + 2);
	int end_row = std::min(canvas_h, start_row + area_h / ts + 2);

	static constexpr Color FLOOR_BG = { 70, 58, 42, 255 };
	static constexpr Color GRID_COLOR = { 80, 80, 110, 200 };

	for (int row = start_row; row < end_row; ++row)
	{
		for (int col = start_col; col < end_col; ++col)
		{
			int px = area_x + col * ts - pan_x;
			int py = area_y + row * ts - pan_y;

			// Base layer (structural)
			int base_tile_id = canvas_tile_id(col, row);
			if (base_tile_id != 0)
				r.draw_tile_screen(px, py, base_tile_id);
			else
				DrawRectangle(px, py, ts, ts, FLOOR_BG);

			// Decor layer -- additively blended so black sprite pixels
			// are transparent and the base tile shows through underneath.
			char decor_sym = (row < static_cast<int>(decor_canvas.size()) &&
								 col < static_cast<int>(decor_canvas[row].size()))
				? decor_canvas[row][col]
				: ' ';

			if (decor_sym != ' ')
			{
				int decor_tile_id = symbol_tile_id(decor_sym);
				if (decor_tile_id != 0)
					r.draw_tile_screen(px, py, decor_tile_id);
			}

			DrawRectangleLines(px, py, ts, ts, GRID_COLOR);
		}
	}

	EndScissorMode();

	// Panel borders
	DrawLine(area_x, area_y, area_x, area_y + area_h, Color{ 70, 70, 110, 255 });
	DrawLine(area_x + area_w, area_y, area_x + area_w, area_y + area_h, Color{ 70, 70, 110, 255 });

	// Canvas size label in top-left corner of canvas area
	std::string dim_label = std::format("{}x{}", canvas_w, canvas_h);
	r.draw_text_color(area_x + 4, area_y + 4, dim_label, Color{ 100, 100, 140, 160 });
}

void RoomEditor::render_right_panel(const Renderer& r) const
{
	if (!library)
		return;

	int ts = r.get_tile_size();
	int panel_x = r.get_screen_width() - RIGHT_W_TILES * ts;
	int panel_y = TOP_H_TILES * ts;
	int panel_h = r.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * ts;
	int panel_w = RIGHT_W_TILES * ts;

	DrawRectangle(panel_x, panel_y, panel_w, panel_h, Color{ 15, 15, 22, 230 });
	DrawLine(panel_x, panel_y, panel_x, panel_y + panel_h, Color{ 70, 70, 110, 255 });

	// Header
	r.draw_text_color(panel_x + 6, panel_y + 4, "SAVED PREFABS", Color{ 160, 160, 220, 255 });

	int font_h = r.get_font_size() + 4;
	int list_y_off = font_h + 4;
	int list_h = panel_h - list_y_off;
	int visible = list_h / font_h;
	const auto& all = library->all();
	int count = static_cast<int>(all.size());

	for (int slot = 0; slot < visible && (list_scroll + slot) < count; ++slot)
	{
		int idx = list_scroll + slot;
		int py = panel_y + list_y_off + slot * font_h;

		bool selected = (idx == list_selected);

		if (selected)
			DrawRectangle(panel_x, py, panel_w, font_h, Color{ 50, 50, 90, 200 });

		const Prefab& p = all[idx];
		std::string label = std::format("{}  {}x{}", p.name, p.width(), p.height());

		Color entry_col = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 200, 200 };

		r.draw_text_color(panel_x + 6, py + 2, label, entry_col);
	}

	// Empty state
	if (count == 0)
	{
		r.draw_text_color(
			panel_x + 6, panel_y + list_y_off + 4, "No prefabs yet.", Color{ 100, 100, 120, 150 });
	}

	// Workflow guidance -- drawn in the bottom portion of the right panel
	int guide_lines = 8;
	int guide_h = guide_lines * font_h + 8;
	int guide_y = panel_y + panel_h - guide_h;

	DrawLine(panel_x, guide_y, panel_x + panel_w, guide_y, Color{ 60, 60, 100, 200 });
	DrawRectangle(panel_x, guide_y + 1, panel_w, guide_h - 1, Color{ 10, 10, 18, 200 });

	r.draw_text_color(panel_x + 6, guide_y + 4, "HOW IT WORKS", Color{ 140, 140, 220, 255 });

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
		r.draw_text_color(panel_x + 6, guide_text_y, line, guide_col);
		guide_text_y += font_h;
	}
}

void RoomEditor::render_bottom_bar(const Renderer& r) const
{
	int ts = r.get_tile_size();
	int sw = r.get_screen_width();
	int bar_y = r.get_screen_height() - ts;

	DrawRectangle(0, bar_y, sw, ts, Color{ 20, 20, 30, 240 });
	DrawLine(0, bar_y, sw, bar_y, Color{ 80, 80, 120, 255 });

	std::string_view controls =
		"L=paint  R=erase  Mid=pan  Up/Dn=pal  +/-=zoom  F2=tile  F3=label  Ctrl+S=save  Esc=exit";

	r.draw_text_color(8, bar_y + (ts - r.get_font_size()) / 2, controls, Color{ 140, 140, 180, 220 });
}

void RoomEditor::render_input_overlay(const Renderer& r) const
{
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();

	// Semi-transparent backdrop
	DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 160 });

	std::string prompt;
	if (mode == EditorMode::INPUT_WIDTH)
		prompt = std::format("New canvas width (current {}): {}_", canvas_w, input_buf);
	else if (mode == EditorMode::INPUT_HEIGHT)
		prompt = std::format("New canvas height (current {}): {}_", canvas_h, input_buf);
	else if (mode == EditorMode::INPUT_NAME)
		prompt = std::format("Prefab name: {}_", input_buf);
	else if (mode == EditorMode::INPUT_LABEL)
		prompt = std::format("Symbol label: {}_", input_buf);

	int font_h = r.get_font_size();
	int text_w = r.measure_text(prompt);
	int box_w = text_w + 32;
	int box_h = font_h + 20;
	int box_x = (sw - box_w) / 2;
	int box_y = (sh - box_h) / 2;

	DrawRectangle(box_x, box_y, box_w, box_h, Color{ 20, 20, 35, 240 });
	DrawRectangleLines(box_x, box_y, box_w, box_h, Color{ 120, 120, 200, 255 });
	r.draw_text_color(box_x + 16, box_y + 10, prompt, Color{ 220, 220, 255, 255 });
}

// ---------------------------------------------------------------------------
// Tile picker (F2)
// ---------------------------------------------------------------------------

// Sheets available in the picker, in Tab-cycle order.
static constexpr int PICKER_SHEETS[] = {
	SHEET_DECOR0, SHEET_DECOR1, SHEET_FLOOR, SHEET_WALL, SHEET_TILE
};
static constexpr int PICKER_SHEET_COUNT = 5;
static constexpr const char* PICKER_SHEET_NAMES[] = {
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
	int sheet_id = PICKER_SHEETS[picker_sheet_list_idx];
	int cols = std::max(1, r.get_sheet_cols(sheet_id));
	int rows = std::max(1, r.get_sheet_rows(sheet_id));

	if (IsKeyPressed(KEY_TAB))
	{
		int dir = (IsKeyDown(KEY_LEFT_SHIFT)) ? -1 : 1;
		picker_sheet_list_idx = (picker_sheet_list_idx + PICKER_SHEET_COUNT + dir) % PICKER_SHEET_COUNT;
		picker_col = 0;
		picker_row = 0;
		return;
	}

	if (IsKeyPressed(KEY_LEFT))
		picker_col = (picker_col - 1 + cols) % cols;
	if (IsKeyPressed(KEY_RIGHT))
		picker_col = (picker_col + 1) % cols;
	if (IsKeyPressed(KEY_UP))
		picker_row = (picker_row - 1 + rows) % rows;
	if (IsKeyPressed(KEY_DOWN))
		picker_row = (picker_row + 1) % rows;

	if (IsKeyPressed(KEY_ENTER) && library)
	{
		const auto& pal = library->ordered_palette();
		if (palette_index >= 0 && palette_index < static_cast<int>(pal.size()))
		{
			char sym = pal[palette_index].first;
			int tile_id = make_tile(sheet_id, picker_col, picker_row);
			library->set_symbol_tile(sym, tile_id);
			library->save(Paths::PREFABS);
			set_status(std::format("'{}' -> tile ({},{}) sheet {}", sym, picker_col, picker_row, PICKER_SHEET_NAMES[picker_sheet_list_idx]));
		}
		mode = EditorMode::NORMAL;
	}
}

void RoomEditor::render_tile_picker(const Renderer& r) const
{
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();

	// Dark overlay over editor
	DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 200 });

	// Picker tile display size -- 2x native for visibility
	static constexpr int PICK_TS = SPRITE_SIZE * 2;

	int sheet_id = PICKER_SHEETS[picker_sheet_list_idx];
	int cols = std::max(1, r.get_sheet_cols(sheet_id));
	int rows = std::max(1, r.get_sheet_rows(sheet_id));

	// Center the grid
	int grid_w = cols * PICK_TS;
	int grid_h = rows * PICK_TS;
	int off_x = (sw - grid_w) / 2;
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
			int tile_id = make_tile(sheet_id, col, row);

			// Dark bg so decor tiles are visible
			DrawRectangle(px, py, PICK_TS, PICK_TS, Color{ 20, 16, 12, 255 });

			// Draw at fixed PICK_TS size -- use a manual DrawTexturePro call
			// through the tile_id so we stay consistent with Renderer's sheet.
			// We have draw_tile_screen which uses current tile_size; instead
			// we call r.draw_tile_screen and scale using scissor trick.
			// Simplest: just use r.draw_tile_screen at current tile_size and
			// then undo -- but tile_size might differ.  Use a small wrapper:
			// draw at native scaled to PICK_TS manually.

			// For simplicity rely on r.draw_tile_screen at current tile_size;
			// if tile_size != PICK_TS the tiles will be wrong size.
			// Instead: direct DrawTexturePro using make_tile decomposition.
			Rectangle src = {
				static_cast<float>(tile_col(tile_id) * SPRITE_SIZE),
				static_cast<float>(tile_row(tile_id) * SPRITE_SIZE),
				static_cast<float>(SPRITE_SIZE),
				static_cast<float>(SPRITE_SIZE)
			};
			Rectangle dst = {
				static_cast<float>(px),
				static_cast<float>(py),
				static_cast<float>(PICK_TS),
				static_cast<float>(PICK_TS)
			};
			(void)src;
			(void)dst;
			r.draw_tile_screen_sized(px, py, tile_id, PICK_TS);

			// Selected cell highlight
			if (col == picker_col && row == picker_row)
				DrawRectangleLines(px, py, PICK_TS, PICK_TS, Color{ 255, 230, 50, 255 });
		}
	}

	// Info bar at top
	const auto* pal_ptr = library ? &library->ordered_palette() : nullptr;
	std::string sym_info = "?";
	if (pal_ptr && palette_index >= 0 &&
		palette_index < static_cast<int>(pal_ptr->size()))
	{
		char sym = (*pal_ptr)[palette_index].first;
		sym_info = std::format("Assigning tile for '{}' ({})",
			sym,
			library->symbol_label(sym));
	}

	std::string header = std::format(
		"TILE PICKER  |  {}  |  Sheet {}/{}  [Tab=next  Shift+Tab=prev]  "
		"|  col={} row={}  tile_id={}  |  [Enter]=assign  [Esc]=cancel",
		sym_info,
		picker_sheet_list_idx + 1,
		PICKER_SHEET_COUNT,
		picker_col,
		picker_row,
		make_tile(sheet_id, picker_col, picker_row));

	DrawRectangle(0, 0, sw, 40, Color{ 20, 20, 35, 240 });
	r.draw_text_color(8, 10, header, Color{ 220, 220, 255, 255 });

	// Sheet name label centered below grid
	int name_y = off_y + grid_h + 8;
	std::string sheet_label = std::format("Sheet: {}  ({} cols x {} rows)",
		PICKER_SHEET_NAMES[picker_sheet_list_idx],
		cols,
		rows);
	r.draw_text_color(off_x, name_y, sheet_label, Color{ 160, 160, 220, 200 });
}
