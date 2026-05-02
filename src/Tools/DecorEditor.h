// file: DecorEditor.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Renderer/Renderer.h"

// ---------------------------------------------------------------------------
// DecorEditor -- in-game decoration placement and tile labeling tool.
//
// F2           toggle editor mode
// Tab          toggle sprite sheet browser
// , / .        cycle palette tile
// Left click   place current palette tile at hovered world tile
// Right click  erase decoration at hovered world tile
// Ctrl+S       save overrides + palette
//
// Browser mode (Tab):
//   Left/Right arrows   cycle sprite sheets
//   Wheel (in grid)     scroll tile grid
//   Wheel (in list)     scroll palette list
//   Click list entry    select entry -- jumps to its sheet, enters reassign mode
//   Click tile (normal) select tile for new label / symbol
//   Click tile (reassign) reassign sprite of the selected palette entry
//   Tab                 switch between label and symbol fields
//   Enter               save label / symbol (and sprite if reassigned)
//   Escape              cancel -- clear selection
//   Delete              remove selected tile from palette
// ---------------------------------------------------------------------------
class DecorEditor
{
public:
	DecorEditor();

	struct PaletteEntry
	{
		TileRef tile{};
		std::string label{};
		char symbol{ 0 }; // 0 = no symbol (tile not usable in prefabs)
	};

	void toggle();
	void set_char_input(int ch) noexcept { buffered_char = ch; }
	void cycle_next();
	void cycle_prev();

	void set_active_map(long seed, int dungeon_level);

	// Palette persistence -- both read/write the "palette" array in tile_config.json.
	void save_palette(std::string_view path) const;
	void load_palette(std::string_view path);

	// Called every render frame while active.
	void update_and_render(const Renderer& renderer);

	[[nodiscard]] bool is_active() const { return active; }
	[[nodiscard]] bool is_browser_open() const { return browser_open; }

	[[nodiscard]] TileRef get_override(int world_x, int world_y) const;

	void place(int world_x, int world_y);
	void erase(int world_x, int world_y);

	// Direct tile placement bypassing the palette -- used by PrefabLibrary.
	void place_tile(int world_x, int world_y, TileRef tile);

	[[nodiscard]] bool is_active_map_empty() const;

	[[nodiscard]] const std::vector<PaletteEntry>& get_palette() const { return palette; }

private:
	bool active{ false };
	int palette_index{ 0 };
	mutable double last_save_time{ -10.0 };

	std::string active_key;
	std::string palette_path; // set during load_palette, used by save_palette and browser

	std::unordered_map<std::string, std::unordered_map<uint32_t, TileRef>> all_overrides;

	// Dynamic palette -- loaded from tile_labels.json, editable at runtime.
	std::vector<PaletteEntry> palette;

	// Sheet browser state
	bool browser_open{ false };
	int browser_sheet{ 0 };
	int browser_scroll{ 0 };
	TileRef browser_selected{}; // selected tile, is_valid() == false means none

	// Palette list panel state
	int list_cursor{ -1 }; // selected palette index; -1 = none
	int list_scroll{ 0 };

	// Label / symbol editing state
	bool editing{ false };
	std::string label_buf;
	char sym_buf{ 0 };
	bool editing_sym{ false }; // true = cursor in symbol field
	bool label_all_selected{ false }; // first keystroke replaces entire label
	int buffered_char{ 0 }; // char polled by InputSystem::poll(), fed in before render

	[[nodiscard]] static uint32_t make_key(int x, int y) noexcept
	{
		return (static_cast<uint32_t>(y) << 16) | static_cast<uint32_t>(x & 0xFFFF);
	}

	[[nodiscard]] std::unordered_map<uint32_t, TileRef>& current_map();
	[[nodiscard]] const std::unordered_map<uint32_t, TileRef>& current_map() const;

	// Normal editor overlay
	void draw_cursor(const Renderer& renderer, int world_x, int world_y) const;
	void draw_palette_strip(const Renderer& renderer) const;
	void draw_info_bar(const Renderer& renderer, int world_x, int world_y) const;

	// Sheet browser
	void update_browser(const Renderer& renderer, std::string_view palette_path);

	// Palette helpers
	[[nodiscard]] std::optional<int> palette_find(TileRef tile) const;
	void palette_add_or_update(TileRef tile, std::string_view label, char symbol);
	void palette_remove(TileRef tile);
};
