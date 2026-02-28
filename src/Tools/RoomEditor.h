#pragma once
// file: RoomEditor.h
//
// Full-screen in-game room authoring tool.
// Entered from the main menu ("Room Editor" option).
// Produces Prefab objects saved to prefabs.json via PrefabLibrary.
//
// Controls (handled via direct raylib calls, not through InputSystem):
//   Up / Down       -- cycle palette selection
//   Left click      -- paint selected symbol on canvas
//   Right click     -- erase (set to '.' floor)
//   Middle drag     -- pan canvas
//   +  / -          -- zoom (global tile_size)
//   N               -- new canvas (enters width/height input mode)
//   Ctrl+S          -- save current canvas as prefab (enters name input mode)
//   Enter (list)    -- load selected prefab from right panel
//   Del             -- delete selected prefab from library
//   Esc             -- exit Room Editor, return to menu

#include <string>
#include <vector>

#include "../Renderer/Renderer.h"

class PrefabLibrary;
struct Prefab;
struct GameContext;

class RoomEditor
{
public:
	void enter(PrefabLibrary& lib);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }

private:
	enum class EditorMode
	{
		NORMAL,
		INPUT_WIDTH,
		INPUT_HEIGHT,
		INPUT_NAME,
		INPUT_LABEL, // F3: rename palette symbol label
		TILE_PICKER // F2: browse sprite sheet to assign tile to symbol
	};

	bool active{ false };
	PrefabLibrary* library{ nullptr };

	// Canvas (two layers)
	std::vector<std::string> canvas; // base layer: structural symbols (#  .  ,  +  ~)
	std::vector<std::string> decor_canvas; // decor layer: non-structural overlays (' ' = empty)
	int canvas_w{ 12 };
	int canvas_h{ 8 };

	// Prefab metadata
	std::string prefab_name{ "unnamed" };
	int depth_min{ 1 };
	int depth_max{ 10 };
	float weight{ 1.0f };

	// Palette state
	int palette_index{ 0 };

	// Canvas pan (pixels)
	int pan_x{ 0 };
	int pan_y{ 0 };

	// Mouse pan tracking
	bool panning{ false };
	float pan_mouse_start_x{ 0.0f };
	float pan_mouse_start_y{ 0.0f };
	int pan_start_x{ 0 };
	int pan_start_y{ 0 };

	// Right panel list
	int list_scroll{ 0 };
	int list_selected{ -1 };

	// Status flash
	std::string status_msg;
	double status_time{ -10.0 };

	// Input mode
	EditorMode mode{ EditorMode::NORMAL };
	std::string input_buf;
	int pending_w{ 0 }; // holds entered width while awaiting height

	// Tile picker state
	int picker_sheet_list_idx{ 0 }; // index into PICKER_SHEETS[]
	int picker_col{ 0 };
	int picker_row{ 0 };

	// Layout (all in tiles; pixels = value * tile_size)
	static constexpr int LEFT_W_TILES = 11;
	static constexpr int RIGHT_W_TILES = 13;
	static constexpr int TOP_H_TILES = 1;
	static constexpr int BOT_H_TILES = 1;

	void handle_input(GameContext& ctx);
	void handle_input_normal(GameContext& ctx);
	void handle_input_text(GameContext& ctx);
	void handle_input_picker(GameContext& ctx);

	void render(const GameContext& ctx) const;
	void render_top_bar(const Renderer& r) const;
	void render_left_panel(const Renderer& r) const;
	void render_canvas(const Renderer& r) const;
	void render_right_panel(const Renderer& r) const;
	void render_bottom_bar(const Renderer& r) const;
	void render_input_overlay(const Renderer& r) const;
	void render_tile_picker(const Renderer& r) const;

	void new_canvas(int w, int h);
	void do_save(const std::string& name);
	void do_load(int prefab_index);
	void do_delete(int prefab_index);
	void set_status(const std::string& msg);

	// Returns canvas cell coords from screen pixel, false if outside canvas.
	[[nodiscard]] bool screen_to_canvas(
		const Renderer& r,
		int mouse_px,
		int mouse_py,
		int& out_cx,
		int& out_cy) const;

	// Returns prefab list index from screen pixel in right panel, -1 if none.
	[[nodiscard]] int screen_to_list_index(const Renderer& r, int mouse_px, int mouse_py) const;

	// Returns palette index from screen pixel in left panel, -1 if none.
	[[nodiscard]] int screen_to_palette_index(const Renderer& r, int mouse_px, int mouse_py) const;

	// Tile to draw for a symbol in the canvas and palette.
	[[nodiscard]] TileRef symbol_tile_id(char sym) const;

	// Position-aware tile that resolves wall/floor auto-tiling.
	[[nodiscard]] TileRef canvas_tile_id(int col, int row) const;

	// 4-bit NESW bitmask: neighbour is '#' (or out-of-bounds) = wall.
	[[nodiscard]] int canvas_wall_mask(int col, int row) const;

	// Current selected symbol character.
	[[nodiscard]] char selected_sym() const;
};
