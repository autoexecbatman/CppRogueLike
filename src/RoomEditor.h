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

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Renderer.h"

class PrefabLibrary;
class TileConfig;
struct Prefab;
struct GameContext;

// The saved-prefab list's rows: where each one is drawn and which one a pixel falls in.
// The draw and the click both read this, so a click selects the row drawn under it.
//
// Example:
//   const PrefabListRows rows{ 64, 600, 16 };   // panel top, panel height, font size
//   rows.row_height();    // -> 20
//   rows.row_top(0);      // -> 88, below the "SAVED PREFABS" header line
//   rows.visible_rows();  // -> 28, whole rows that fit under the header
//   rows.slot_at(100);    // -> 0
//   rows.slot_at(70);     // -> nullopt, the header
//   rows.slot_at(648);    // -> nullopt, past the last whole row
class PrefabListRows
{
private:
	int top{ 0 };
	int rowHeight{ 0 };
	size_t visibleRows{ 0 };

public:
	// The rows of a list whose panel starts at panelTop and is panelHeight pixels tall, at the given font size.
	PrefabListRows(int panelTop, int panelHeight, int fontSize);

	// The top pixel of the drawn slot, counted from the first row on screen.
	[[nodiscard]] int row_top(size_t slot) const;
	// A row's height in pixels: the font plus 4.
	[[nodiscard]] int row_height() const;
	// The whole rows that fit below the header; a cut-off row at the bottom is not counted.
	[[nodiscard]] size_t visible_rows() const;

	// The drawn slot under a pixel row, or nullopt on the header or past the last whole row.
	[[nodiscard]] std::optional<size_t> slot_at(int pixelY) const;
};

class RoomEditor
{
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
	const TileConfig* tileConfig{ nullptr };

	// Canvas (two layers)
	std::vector<std::string> canvas; // base layer: structural symbols (#  .  ,  +  ~)
	std::vector<std::string> decorCanvas; // decor layer: non-structural overlays (' ' = empty)
	int canvasWidth{ 12 };
	int canvasHeight{ 8 };

	// Prefab metadata
	std::string prefabName{ "unnamed" };
	int depthMin{ 1 };
	int depthMax{ 10 };
	float weight{ 1.0f };

	// Palette state
	size_t paletteIndex{ 0 };

	// Canvas pan (pixels)
	int panX{ 0 };
	int panY{ 0 };

	// Mouse pan tracking
	bool panning{ false };
	float panMouseStartX{ 0.0f };
	float panMouseStartY{ 0.0f };
	int panStartX{ 0 };
	int panStartY{ 0 };

	// Right panel list
	size_t listScroll{ 0 };
	std::optional<size_t> listSelected{}; // the prefab chosen in the list, if any

	// Status flash
	std::string statusMsg;
	double statusTime{ -10.0 };

	// Input mode
	EditorMode mode{ EditorMode::NORMAL };
	std::string inputBuffer;
	int pendingWidth{ 0 }; // holds entered width while awaiting height

	// Tile picker state
	int pickerSheetListIndex{ 0 }; // index into PICKER_SHEETS[]
	int pickerCol{ 0 };
	int pickerRow{ 0 };

	void handle_input(GameContext& ctx);
	void handle_input_normal(GameContext& ctx);
	void handle_input_text();
	void handle_input_picker(GameContext& ctx);

	void render(const GameContext& ctx) const;
	void render_top_bar(const Renderer& renderer) const;
	void render_left_panel(const Renderer& renderer) const;
	void render_canvas(const Renderer& renderer) const;
	void render_right_panel(const Renderer& renderer) const;
	void render_bottom_bar(const Renderer& renderer) const;
	void render_input_overlay(const Renderer& renderer) const;
	void render_tile_picker(const Renderer& renderer) const;

	void new_canvas(int width, int height);
	void do_save(const std::string& name);
	void do_load(size_t prefabIndex);
	void do_delete(size_t prefabIndex);
	void set_status(const std::string& message);

	// A cell of the canvas, by column and row.
	struct CanvasCell
	{
		int col{ 0 };
		int row{ 0 };
	};

	// The canvas cell under a screen pixel, or nullopt when the pixel is outside the canvas area or past the canvas's edge.
	[[nodiscard]] std::optional<CanvasCell> screen_to_canvas(
		const Renderer& renderer,
		int mousePx,
		int mousePy) const;
	[[nodiscard]] std::optional<size_t> screen_to_list_index(const Renderer& renderer, int mousePx, int mousePy) const;
	[[nodiscard]] std::optional<size_t> screen_to_palette_index(const Renderer& renderer, int mousePx, int mousePy) const;
	[[nodiscard]] TileRef symbol_tile_id(char symbol) const;
	[[nodiscard]] TileRef canvas_tile_id(int col, int row) const;
	[[nodiscard]] int canvas_wall_mask(int col, int row) const;
	[[nodiscard]] char selected_sym() const;

public:
	void enter(PrefabLibrary& lib);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
