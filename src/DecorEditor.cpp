// file: DecorEditor.cpp
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <raylib.h>

#include "DecorEditor.h"
#include "Paths.h"
#include "Renderer.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------

DecorEditor::DecorEditor() = default;

// ---------------------------------------------------------------------------
// Palette helpers
// ---------------------------------------------------------------------------

std::optional<int> DecorEditor::palette_find(TileRef tile) const
{
	for (int entryIndex = 0; entryIndex < static_cast<int>(palette.size()); ++entryIndex)
	{
		if (palette[entryIndex].tile == tile)
		{
			return entryIndex;
		}
	}
	return std::nullopt;
}

void DecorEditor::palette_add_or_update(
	TileRef tile,
	std::string_view label,
	char symbol)
{
	auto idx = palette_find(tile);
	if (idx)
	{
		palette[*idx].label = label;
		palette[*idx].symbol = symbol;
	}
	else
	{
		palette.push_back({ tile, std::string(label), symbol });
	}
}

void DecorEditor::palette_remove(TileRef tile)
{
	std::erase_if(palette,
		[tile](const PaletteEntry& entry)
		{ return entry.tile == tile; });

	if (palette_index >= static_cast<int>(palette.size()))
	{
		palette_index = std::max(0, static_cast<int>(palette.size()) - 1);
	}
}

// ---------------------------------------------------------------------------
// nlohmann serialization for PaletteEntry
// tile is stored as human-readable sheet/col/row.
// ---------------------------------------------------------------------------

void to_json(json& output, const DecorEditor::PaletteEntry& entry)
{
	output = {
		{ "sheet", static_cast<int>(entry.tile.sheet) },
		{ "col", entry.tile.col },
		{ "row", entry.tile.row },
		{ "label", entry.label },
		{ "symbol", entry.symbol ? std::string(1, entry.symbol) : std::string{} }
	};
}

void from_json(const json& source, DecorEditor::PaletteEntry& entry)
{
	entry.tile = TileRef{
		static_cast<TileSheet>(source.value("sheet", 0)),
		source.value("col", -1),
		source.value("row", -1)
	};
	entry.label = source.value("label", "");
	std::string sym = source.value("symbol", "");
	entry.symbol = sym.empty() ? 0 : sym[0];
}

// ---------------------------------------------------------------------------
// Persistence -- palette
// ---------------------------------------------------------------------------

void DecorEditor::save_palette(std::string_view path) const
{
	auto abs = Paths::resolve(path);

	// Read the existing file so other sections are preserved.
	json document = json::object();
	{
		std::ifstream existingFile(abs);
		if (existingFile.is_open())
		{
			try
			{
				existingFile >> document;
			}
			catch (const json::exception& error)
			{
				std::clog << std::format("[DecorEditor] tile_config.json error, starting fresh: {}\n", error.what());
				document = json::object();
			}
		}
	}

	document["palette"] = palette;

	std::filesystem::create_directories(abs.parent_path());
	std::ofstream out(abs);
	out << document.dump(4);
	last_save_time = GetTime();
	std::clog << std::format("[DecorEditor] palette saved: {}\n", abs.string());
}

void DecorEditor::load_palette(std::string_view path)
{
	palette_path = std::string(path);
	palette.clear();
	palette_index = 0;

	auto abs = Paths::resolve(path);
	std::ifstream paletteFile(abs);
	if (!paletteFile.is_open())
	{
		std::clog << std::format("[DecorEditor] palette not found: {}\n", abs.string());
		return;
	}
	std::clog << std::format("[DecorEditor] palette loaded: {}\n", abs.string());

	try
	{
		json document;
		paletteFile >> document;
		if (!document.contains("palette"))
		{
			return;
		}

		palette = document["palette"].get<std::vector<PaletteEntry>>();
		std::erase_if(palette,
			[](const PaletteEntry& entry)
			{ return entry.label.empty(); });
	}
	catch (const json::exception& error)
	{
		std::clog << std::format("[DecorEditor] palette error, clearing: {}\n", error.what());
		palette.clear();
	}
}

// ---------------------------------------------------------------------------

void DecorEditor::set_active_map(long seed, int dungeon_level)
{
	active_key = std::format("{}_{}", seed, dungeon_level);
}

void DecorEditor::toggle()
{
	active = !active;
	if (!active)
	{
		browser_open = false;
		editing = false;
	}
}

void DecorEditor::cycle_next()
{
	if (palette.empty())
	{
		return;
	}

	palette_index = (palette_index + 1) % static_cast<int>(palette.size());
}

void DecorEditor::cycle_prev()
{
	if (palette.empty())
	{
		return;
	}

	int paletteSize = static_cast<int>(palette.size());
	palette_index = (palette_index - 1 + paletteSize) % paletteSize;
}

void DecorEditor::place(Vector2D worldPosition)
{
	if (palette.empty())
	{
		return;
	}

	current_map()[make_key(worldPosition)] = palette[palette_index].tile;
}

void DecorEditor::erase(Vector2D worldPosition)
{
	current_map().erase(make_key(worldPosition));
}

void DecorEditor::place_tile(Vector2D worldPosition, TileRef tile)
{
	current_map()[make_key(worldPosition)] = tile;
}

bool DecorEditor::is_active_map_empty() const
{
	return current_map().empty();
}

TileRef DecorEditor::get_override(Vector2D worldPosition) const
{
	const auto& overrides = current_map();
	const auto key = make_key(worldPosition);
	return overrides.contains(key) ? overrides.at(key) : TileRef{};
}

std::unordered_map<uint32_t, TileRef>& DecorEditor::current_map()
{
	return all_overrides[active_key];
}

const std::unordered_map<uint32_t, TileRef>& DecorEditor::current_map() const
{
	if (all_overrides.contains(active_key))
	{
		return all_overrides.at(active_key);
	}
	static const std::unordered_map<uint32_t, TileRef> empty;
	return empty;
}

// ---------------------------------------------------------------------------
// update_and_render
// ---------------------------------------------------------------------------

void DecorEditor::update_and_render(const Renderer& renderer)
{
	if (!active)
	{
		return;
	}

	if (IsKeyPressed(KEY_TAB) && !editing)
	{
		browser_open = !browser_open;
		if (!browser_open)
		{
			editing = false;
			save_palette(palette_path);
		}
	}

	if (browser_open)
	{
		update_browser(renderer, palette_path);
		return;
	}

	// Normal editor overlay
	const int tile_size = renderer.get_tile_size();
	const int cam_x = renderer.get_camera_x();
	const int cam_y = renderer.get_camera_y();

	::Vector2 mouse_pos = GetMousePosition();
	const Vector2D worldPosition{
		(static_cast<int>(mouse_pos.x) + cam_x) / tile_size,
		(static_cast<int>(mouse_pos.y) + cam_y) / tile_size
	};

	draw_cursor(renderer, worldPosition);
	draw_palette_strip(renderer);
	draw_info_bar(renderer, worldPosition);
}

// ---------------------------------------------------------------------------
// Normal editor drawing
// ---------------------------------------------------------------------------

void DecorEditor::draw_cursor(const Renderer& renderer, Vector2D worldPosition) const
{
	if (palette.empty())
	{
		return;
	}

	const int tile_size = renderer.get_tile_size();
	const int cam_x = renderer.get_camera_x();
	const int cam_y = renderer.get_camera_y();

	int pixelX = worldPosition.x * tile_size - cam_x;
	int pixelY = worldPosition.y * tile_size - cam_y;

	DrawRectangle(pixelX, pixelY, tile_size, tile_size, Color{ 255, 255, 0, 60 });
	DrawRectangleLines(pixelX, pixelY, tile_size, tile_size, Color{ 255, 255, 0, 220 });

	renderer.draw_tile(Vector2D{ worldPosition.x, worldPosition.y }, palette[palette_index].tile, Color{ 255, 255, 255, 180 });
}

void DecorEditor::draw_palette_strip(const Renderer& renderer) const
{
	if (palette.empty())
	{
		return;
	}

	const int tile_size = renderer.get_tile_size();
	const int screenWidth = renderer.get_screen_width();
	const int paletteSize = static_cast<int>(palette.size());

	constexpr int VISIBLE = 9;
	int half = VISIBLE / 2;

	DrawRectangle(0, 0, screenWidth, tile_size + 4, Color{ 0, 0, 0, 200 });

	for (int slot = 0; slot < VISIBLE; ++slot)
	{
		int idx = palette_index - half + slot;
		if (idx < 0 || idx >= paletteSize)
		{
			continue;
		}

		int slotPixelX = slot * tile_size + (screenWidth - VISIBLE * tile_size) / 2;
		int slotPixelY = 2;

		bool is_current = (idx == palette_index);

		if (is_current)
		{
			DrawRectangle(slotPixelX, slotPixelY, tile_size, tile_size, Color{ 255, 255, 0, 80 });
		}

		renderer.draw_tile_screen(Vector2D{ slotPixelX, slotPixelY }, palette[idx].tile);

		if (is_current)
		{
			DrawRectangleLines(slotPixelX, slotPixelY, tile_size, tile_size, Color{ 255, 255, 0, 255 });
		}
	}
}

void DecorEditor::draw_info_bar(const Renderer& renderer, Vector2D worldPosition) const
{
	const int tile_size = renderer.get_tile_size();
	const int screenWidth = renderer.get_screen_width();

	int bar_y = tile_size + 6;
	DrawRectangle(0, bar_y, screenWidth, 20, Color{ 0, 0, 0, 180 });

	bool saved_flash = (GetTime() - last_save_time) < 2.0;

	std::string tile_name = palette.empty()
		? "(no tiles -- Tab to open browser)"
		: palette[palette_index].label;

	std::string info = std::format(
		"DECOR EDITOR  |  {}  |  Map: {}  |  Pos: {},{}"
		"  |  Tab browser  , / . cycle  LClick place  RClick erase  Ctrl+S save{}",
		tile_name,
		active_key.empty() ? "none" : active_key,
		worldPosition.x,
		worldPosition.y,
		saved_flash ? "  -- SAVED!" : "");

	Color text_color = saved_flash
		? Color{ 100, 255, 100, 255 }
		: Color{ 255, 255, 180, 255 };

	renderer.draw_text_color(Vector2D{ 4, bar_y + 2 }, info, text_color);
}

// ---------------------------------------------------------------------------
// Sheet browser
// ---------------------------------------------------------------------------

void DecorEditor::update_browser(const Renderer& renderer, std::string_view palette_path)
{
	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();

	constexpr int BROWSER_TILE = 32;
	constexpr int LIST_TILE = 20;
	constexpr int LIST_ITEM_H = 28;
	constexpr int LIST_W = 320;
	constexpr int PAD = 8;
	constexpr int HEADER_H = 50;
	constexpr int EDIT_H = 104;

	// Full-screen dark backdrop
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 220 });

	// --- Sheet navigation (Left/Right arrow) ---
	const int total_sheets = renderer.get_loaded_sheet_count();
	auto advance_sheet = [&](int dir)
	{
		for (int sheetsTried = 0; sheetsTried < total_sheets; ++sheetsTried)
		{
			browser_sheet = (browser_sheet + dir + total_sheets) % total_sheets;
			if (renderer.sheet_is_loaded(static_cast<TileSheet>(browser_sheet)))
			{
				break;
			}
		}
		browser_scroll = 0;
		browser_selected = TileRef{};
		editing = false;
	};

	if (IsKeyPressed(KEY_LEFT))
	{
		advance_sheet(-1);
	}
	if (IsKeyPressed(KEY_RIGHT))
	{
		advance_sheet(1);
	}

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(browser_sheet)))
	{
		advance_sheet(1);
	}

	const int sheet_cols = renderer.get_sheet_cols(static_cast<TileSheet>(browser_sheet));
	const int sheet_rows = renderer.get_sheet_rows(static_cast<TileSheet>(browser_sheet));

	// --- Header (two lines) ---
	DrawRectangle(0, 0, screenWidth, HEADER_H, Color{ 20, 20, 40, 255 });

	std::string headerLine = std::format(
		"TILE BROWSER  |  {} ({}/{})  |  {}x{} tiles",
		renderer.get_sheet_name(static_cast<TileSheet>(browser_sheet)),
		browser_sheet + 1,
		total_sheets,
		sheet_cols,
		sheet_rows);

	std::string hintLine = "Left/Right: sheet    Tab: close    Click list entry: rename    [X]: delete    Click tile: add/edit";

	renderer.draw_text_color(Vector2D{ PAD, 6 }, headerLine, Color{ 255, 255, 180, 255 });
	renderer.draw_text_color(Vector2D{ PAD, 28 }, hintLine, Color{ 160, 160, 120, 255 });

	const int grid_y = HEADER_H + 2;
	const int grid_h = screenHeight - grid_y - (editing ? EDIT_H : 0) - 2;

	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	const bool mouse_in_list = mouse.x < LIST_W && mouse.y >= grid_y && mouse.y < grid_y + grid_h;
	const bool mouse_in_grid = mouse.x >= LIST_W;

	// --- Palette list panel ---
	DrawRectangle(0, grid_y, LIST_W, grid_h, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_W, grid_y, LIST_W, grid_y + grid_h, Color{ 80, 80, 120, 255 });
	renderer.draw_text_color(Vector2D{ PAD, grid_y + 4 }, "PALETTE", Color{ 200, 200, 100, 255 });

	const int list_top = grid_y + 20;
	const int list_vis_h = grid_h - 20;
	const int visible_rows = list_vis_h / LIST_ITEM_H;
	const int max_scroll = std::max(0, static_cast<int>(palette.size()) - visible_rows);
	list_scroll = std::min(list_scroll, max_scroll);

	if (mouse_in_list)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			list_scroll = std::clamp(list_scroll - static_cast<int>(wheel), 0, max_scroll);
		}
	}

	// Delete button column on the right edge of each list item.
	constexpr int DEL_W = 22;
	constexpr int CHAR_W = 8; // approx px per glyph (default raylib font)
	const int text_x_l = PAD + LIST_TILE + 4;
	const int del_x = LIST_W - PAD - DEL_W;
	const int max_chars = std::max(3, (del_x - text_x_l - 4) / CHAR_W);

	auto truncate = [&](std::string text) -> std::string
	{
		if (static_cast<int>(text.size()) > max_chars)
		{
			return text.substr(0, max_chars - 3) + "...";
		}
		return text;
	};

	// The one place the edit panel opens for any TileRef, with all editing state read from the palette.
	auto begin_edit = [this, &renderer](TileRef tid)
	{
		browser_selected = tid;
		auto foundIndex = palette_find(tid);
		if (foundIndex)
		{
			list_cursor = *foundIndex;
			label_buf = palette[*foundIndex].label;
			sym_buf = palette[*foundIndex].symbol;
		}
		else
		{
			list_cursor = -1;
			label_buf = std::format("{} {},{}",
				renderer.get_sheet_name(tid.sheet),
				tid.col,
				tid.row);
			sym_buf = 0;
		}
		editing = true;
		editing_sym = false;
		label_all_selected = true;
	};

	BeginScissorMode(0, list_top, LIST_W, list_vis_h);

	for (int entryIndex = list_scroll; entryIndex < static_cast<int>(palette.size()); ++entryIndex)
	{
		int itemY = list_top + (entryIndex - list_scroll) * LIST_ITEM_H;
		if (itemY >= list_top + list_vis_h)
		{
			break;
		}

		bool is_sel = (entryIndex == list_cursor);
		bool hovered = mouse_in_list && mouse.y >= itemY && mouse.y < itemY + LIST_ITEM_H;

		if (is_sel)
		{
			DrawRectangle(0, itemY, LIST_W, LIST_ITEM_H, Color{ 60, 60, 0, 220 });
		}
		else if (hovered)
		{
			DrawRectangle(0, itemY, LIST_W, LIST_ITEM_H, Color{ 30, 30, 30, 180 });
		}

		renderer.draw_tile_screen_sized(
			Vector2D{ PAD, itemY + (LIST_ITEM_H - LIST_TILE) / 2 },
			palette[entryIndex].tile,
			LIST_TILE);

		// Label -- show live edit buffer inline when this row is being renamed.
		std::string entry_text;
		Color textColor;
		if (is_sel && editing)
		{
			// Show what the user is currently typing, with cursor.
			std::string live = label_all_selected
				? std::format("[{}]", label_buf) // selected-all state
				: std::format("{}|", label_buf); // normal typing cursor
			if (sym_buf)
			{
				live += std::format(" ({})", sym_buf);
			}
			entry_text = truncate(live);
			textColor = Color{ 255, 220, 60, 255 };
		}
		else
		{
			entry_text = palette[entryIndex].label;
			if (palette[entryIndex].symbol)
			{
				entry_text += std::format(" [{}]", palette[entryIndex].symbol);
			}
			entry_text = truncate(entry_text);
			textColor = is_sel ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
		}
		renderer.draw_text_color(Vector2D{ text_x_l, itemY + (LIST_ITEM_H - 16) / 2 }, entry_text, textColor);

		// [X] delete button -- visible on selected or hovered row.
		if (is_sel || hovered)
		{
			int btn_y = itemY + 4;
			int btn_h = LIST_ITEM_H - 8;
			bool del_hot = mouse_in_list && mouse.x >= del_x && mouse.x < del_x + DEL_W && mouse.y >= itemY && mouse.y < itemY + LIST_ITEM_H;

			Color del_bg = del_hot ? Color{ 220, 50, 50, 255 } : Color{ 140, 40, 40, 200 };
			DrawRectangle(del_x, btn_y, DEL_W, btn_h, del_bg);
			renderer.draw_text_color(Vector2D{ del_x + 5, itemY + (LIST_ITEM_H - 16) / 2 }, "X", Color{ 255, 255, 255, 255 });

			if (del_hot && clicked)
			{
				palette_remove(palette[entryIndex].tile);
				save_palette(palette_path);
				list_cursor = -1;
				browser_selected = TileRef{};
				editing = false;
				break; // vector modified -- exit loop safely
			}
		}

		// Entry click (outside [X] button) -- navigate browser to tile's sheet then edit.
		if (hovered && clicked && mouse.x < del_x)
		{
			if (renderer.sheet_is_loaded(palette[entryIndex].tile.sheet))
			{
				browser_sheet = static_cast<int>(palette[entryIndex].tile.sheet);
				browser_scroll = std::max(0, palette[entryIndex].tile.row - 2);
			}
			begin_edit(palette[entryIndex].tile);
		}
	}
	EndScissorMode();

	// --- Tile grid (shifted right by LIST_W) ---
	if (mouse_in_grid)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			browser_scroll -= static_cast<int>(wheel);
			browser_scroll = std::max(0, std::min(browser_scroll, sheet_rows - 1));
		}
	}

	BeginScissorMode(LIST_W, grid_y, screenWidth - LIST_W, grid_h);

	for (int row = browser_scroll; row < sheet_rows; ++row)
	{
		int cellPixelY = grid_y + (row - browser_scroll) * (BROWSER_TILE + 2);
		if (cellPixelY >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int cellPixelX = LIST_W + PAD + col * (BROWSER_TILE + 2);
			TileRef tid{ static_cast<TileSheet>(browser_sheet), col, row };

			bool is_sel = (tid == browser_selected);
			bool in_pal = palette_find(tid).has_value();
			bool hovered_tile = (mouse.x >= cellPixelX && mouse.x < cellPixelX + BROWSER_TILE &&
				mouse.y >= cellPixelY && mouse.y < cellPixelY + BROWSER_TILE);

			if (in_pal)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 20, 60, 20, 200 });
			}
			if (is_sel)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			}
			if (hovered_tile && !is_sel)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 30, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tid, BROWSER_TILE);

			if (is_sel)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
			}
			else if (in_pal)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 220, 60, 200 });
			}

			if (hovered_tile)
			{
				auto foundIndex = palette_find(tid);
				if (foundIndex)
				{
					const auto& entry = palette[*foundIndex];
					std::string tip = entry.label;
					if (entry.symbol)
					{
						tip += std::format(" [{}]", entry.symbol);
					}
					int tip_x = static_cast<int>(mouse.x) + 14;
					int tip_y = static_cast<int>(mouse.y) - 18;
					DrawRectangle(tip_x - 2, tip_y - 2, static_cast<int>(tip.size()) * 8 + 6, 18, Color{ 0, 0, 0, 200 });
					renderer.draw_text_color(Vector2D{ tip_x, tip_y }, tip, Color{ 255, 255, 200, 255 });
				}
			}

			if (hovered_tile && clicked)
			{
				begin_edit(tid);
			}
		}
	}

	EndScissorMode();

	// Delete key
	if (browser_selected.is_valid() && IsKeyPressed(KEY_DELETE))
	{
		palette_remove(browser_selected);
		save_palette(palette_path);
		editing = false;
		browser_selected = TileRef{};
		list_cursor = -1;
	}

	// --- Edit panel ---
	if (editing)
	{
		int panel_y = screenHeight - EDIT_H;
		DrawRectangle(0, panel_y, screenWidth, EDIT_H, Color{ 10, 10, 30, 248 });
		DrawLine(0, panel_y, screenWidth, panel_y, Color{ 100, 100, 200, 255 });

		renderer.draw_tile_screen_sized(
			Vector2D{ PAD, panel_y + (EDIT_H - BROWSER_TILE) / 2 },
			browser_selected,
			BROWSER_TILE);

		int text_x = PAD + BROWSER_TILE + 12;

		// Sprite info (read-only -- set by clicking in browser)
		std::string sprite_info = std::format(
			"Sprite: {} sheet:{} col:{} row:{}",
			renderer.get_sheet_name(browser_selected.sheet),
			static_cast<int>(browser_selected.sheet),
			browser_selected.col,
			browser_selected.row);
		Color sprite_col = list_cursor >= 0
			? Color{ 255, 180, 80, 255 }
			: Color{ 140, 200, 255, 255 };
		renderer.draw_text_color(Vector2D{ text_x, panel_y + 6 }, sprite_info, sprite_col);

		// Label field -- highlighted when all-selected (next keystroke replaces)
		Color lbl_col = !editing_sym
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 80, 255 };
		std::string label_display = label_all_selected
			? std::format("Label: [{}]", label_buf) // brackets = all selected
			: std::format("Label: {}{}", label_buf, !editing_sym ? "|" : "");
		renderer.draw_text_color(Vector2D{ text_x, panel_y + 26 }, label_display, lbl_col);

		// Symbol field
		Color sym_col = editing_sym
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 80, 255 };
		renderer.draw_text_color(
			Vector2D{ text_x, panel_y + 46 }, std::format("Symbol: {}{}", sym_buf ? std::string(1, sym_buf) : "_", editing_sym ? "|" : ""), sym_col);

		renderer.draw_text_color(Vector2D{ text_x, panel_y + 72 }, "Tab=switch field  |  Enter=save  |  Esc=cancel  |  Del=remove", Color{ 140, 140, 140, 255 });

		if (IsKeyPressed(KEY_TAB))
		{
			editing_sym = !editing_sym;
			label_all_selected = false;
		}

		if (IsKeyPressed(KEY_ESCAPE))
		{
			editing = false;
			browser_selected = TileRef{};
			list_cursor = -1;
			label_all_selected = false;
		}

		if (IsKeyPressed(KEY_ENTER) && !label_buf.empty())
		{
			palette_add_or_update(browser_selected, label_buf, sym_buf);
			if (auto found = palette_find(browser_selected))
			{
				list_cursor = *found;
				list_scroll = std::max(0, list_cursor - visible_rows + 1);
			}
			save_palette(palette_path);
			editing = false;
		}

		// The char InputSystem::poll() already took from GetCharPressed(); a second call here returns 0.
		const int pressed = buffered_char;
		buffered_char = 0;

		if (!editing_sym)
		{
			if (IsKeyPressed(KEY_BACKSPACE))
			{
				label_buf.clear();
				label_all_selected = false;
			}

			if (pressed >= 32 && pressed < 127)
			{
				if (label_all_selected)
				{
					label_buf.clear();
					label_all_selected = false;
				}
				label_buf += static_cast<char>(pressed);
			}
		}
		else
		{
			if (IsKeyPressed(KEY_BACKSPACE))
			{
				sym_buf = 0;
			}

			if (pressed > 32 && pressed < 127)
			{
				sym_buf = static_cast<char>(pressed);
			}
		}
	}
}
