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
#include <string_view>
#include <vector>

#include "TileRef.h"

class Palette;
class PrefabLibrary;
class Renderer;
class TileConfig;
struct Prefab;
struct GameContext;

// The saved-prefab list's rows: where each one is drawn and which one a pixel falls in.
// The list runs from below its header to the top of the guide box at the panel's bottom.
// The draw and the click both read this, so a click selects the row drawn under it.
//
// Example:
//   const PrefabListRows rows{ 64, 600, 16 };   // panel top, panel height, font size
//   rows.row_height();    // -> 20
//   rows.row_top(0);      // -> 88, below the "SAVED PREFABS" header line
//   rows.guide_top();     // -> 496, the guide box's heading and lines plus 8 pixels above the panel's bottom
//   rows.visible_rows();  // -> 20, whole rows between the header and the guide box
//   rows.slot_at(100);    // -> 0
//   rows.slot_at(70);     // -> nullopt, the header
//   rows.slot_at(500);    // -> nullopt, under the guide box
class PrefabListRows
{
private:
	int top{ 0 };
	int rowHeight{ 0 };
	int guideTop{ 0 };
	size_t visibleRows{ 0 };

public:
	// The rows of a list whose panel starts at panelTop and is panelHeight pixels tall, at the given font size.
	PrefabListRows(int panelTop, int panelHeight, int fontSize);

	// The top pixel of the drawn slot, counted from the first row on screen.
	[[nodiscard]] int row_top(size_t slot) const;
	// A row's height in pixels: the font plus 4.
	[[nodiscard]] int row_height() const;
	// The top pixel of the guide box, which runs from here to the panel's bottom.
	[[nodiscard]] int guide_top() const;
	// The whole rows that fit between the header and the guide box; a cut-off row is not counted.
	[[nodiscard]] size_t visible_rows() const;

	// The drawn slot under a pixel row, or nullopt on the header or past the last whole row.
	[[nodiscard]] std::optional<size_t> slot_at(int pixelY) const;
};

// The room canvas's cells on screen: where each one is drawn and which one a pixel falls in.
// The draw and the click both read this, so a click paints the cell drawn under it at any pan.
//
// Example:
//   const CanvasGrid grid{ 140, 60, 32 };   // screen pixel of cell (0, 0), tile size
//   grid.cell_left(3);     // -> 236
//   grid.column_at(171);   // -> 0
//   grid.column_at(110);   // -> -1, left of column 0, so no cell
//   grid.row_at(92);       // -> 1
//   CanvasGrid{ 0, 0, 0 }; // aborts: "CanvasGrid: a cell has no size"
class CanvasGrid
{
private:
	int originX{ 0 };
	int originY{ 0 };
	int tileSize{ 0 };

	// The cell an offset from the origin falls in, rounding down so a negative offset is a negative cell.
	[[nodiscard]] int cell_index(int offset) const;

public:
	// A grid whose cell (0, 0) is drawn with its top-left corner at (gridOriginX, gridOriginY).
	CanvasGrid(int gridOriginX, int gridOriginY, int cellSize);

	// The left pixel of a column.
	[[nodiscard]] int cell_left(int col) const;
	// The top pixel of a row.
	[[nodiscard]] int cell_top(int row) const;
	// The column a pixel falls in; negative left of column 0.
	[[nodiscard]] int column_at(int pixelX) const;
	// The row a pixel falls in; negative above row 0.
	[[nodiscard]] int row_at(int pixelY) const;
};

// The whole number typed into a size prompt, or nothing when the text is empty, holds
// anything after the digits, or does not fit an int. A partial number is never read as
// its prefix.
//
// Example:
//   parse_typed_size("12");            // -> 12
//   parse_typed_size("12x");           // -> nullopt, stops before the end
//   parse_typed_size("");              // -> nullopt, the prompt keeps the current size
//   parse_typed_size("99999999999");   // -> nullopt, does not fit an int
[[nodiscard]] std::optional<int> parse_typed_size(std::string_view typed);

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
	// Game declares both before the editor, so they outlive every use of these references.
	PrefabLibrary& library;
	Palette& palette;
	const TileConfig* tileConfig{ nullptr };

	// Canvas (two layers)
	// base layer: structural symbols (#  .  ,  +  ~)
	std::vector<std::string> canvas;
	// decor layer: non-structural overlays (' ' = empty)
	std::vector<std::string> decorCanvas;
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
	// the prefab chosen in the list, if any
	std::optional<size_t> listSelected{};

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

	// Whether a save reached the disk.
	enum class SaveOutcome
	{
		SAVED,
		FAILED
	};

	// Writes the library to prefabs.json. A refused write is put in the status line and reported
	// as FAILED, so the caller shows its own success message only after a save that happened.
	[[nodiscard]] SaveOutcome save_library();

	// Writes the palette to the file it was loaded from, the same way save_library writes the
	// prefabs: a refused write goes in the status line and comes back as FAILED.
	[[nodiscard]] SaveOutcome save_palette();

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
	// An editor of `prefabLibrary`'s prefabs and `editedPalette`'s symbols, inactive until
	// enter. Both must outlive it. It holds them by reference, so it cannot be assigned.
	//
	// Example:
	//   RoomEditor editor{ library, palette };   // inactive: Game ticks it only after enter()
	RoomEditor(PrefabLibrary& prefabLibrary, Palette& editedPalette);

	// Opens the editor on a fresh canvas.
	//
	// Example:
	//   editor.enter();   // is_active() -> true, status() -> ""
	void enter();
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }

	// Renames `symbol` in the palette and saves it. The status line names the new label only
	// after the save; a refused edit or a refused save puts its reason there instead, and a
	// refused edit saves nothing.
	//
	// Example:
	//   editor.commit_symbol_label('W', "Spider web");   // status() -> "Label: 'W'=Spider web"
	//   editor.commit_symbol_label('#', "Stone");        // status() -> "NOT SAVED: # . , + ~ are fixed for ..."
	//   editor.commit_symbol_label('W', "Web");          // file read-only: status() -> "SAVE FAILED: Palette::save -- cannot open '...' for writing"
	void commit_symbol_label(char symbol, std::string_view label);

	// Gives `symbol` a new tile in the palette and saves it, reporting in the status line as
	// commit_symbol_label does.
	//
	// Example:
	//   editor.commit_symbol_tile('W', TileRef{ TileSheet::SHEET_DECOR0, 3, 7 });   // status() -> "'W' -> tile (3,7) sheet Decor0"
	//   editor.commit_symbol_tile('Q', TileRef{ TileSheet::SHEET_DECOR0, 3, 7 });   // status() -> "NOT SAVED: no entry has that symbol"
	void commit_symbol_tile(char symbol, TileRef tile);

	// The status line's text, the last thing an action reported.
	[[nodiscard]] const std::string& status() const noexcept { return statusMsg; }
};
