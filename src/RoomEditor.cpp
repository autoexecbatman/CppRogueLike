// file: RoomEditor.cpp
#include <algorithm>
#include <cassert>
#include <charconv>
#include <format>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <raylib.h>

#include "GameContext.h"
#include "Item.h"
#include "ListCursor.h"
#include "Menu.h"
#include "Palette.h"
#include "Paths.h"
#include "PrefabLibrary.h"
#include "Renderer.h" // includes RaylibIncludes.h -> raylib.h + undefs
#include "RoomEditor.h"
#include "TileConfig.h"

// Sheets available in the picker, in Tab-cycle order.
static constexpr TileSheet PICKER_SHEETS[] = {
	TileSheet::SHEET_DECOR0, TileSheet::SHEET_DECOR1, TileSheet::SHEET_FLOOR, TileSheet::SHEET_WALL, TileSheet::SHEET_TILE
};

static constexpr int PICKER_SHEET_COUNT = 5;

static constexpr std::string_view PICKER_SHEET_NAMES[] = {
	"Decor0", "Decor1", "Floor", "Wall", "Tile"
};

namespace
{
// The picker's name for a sheet, or its number for a sheet the picker does not offer.
//
// Example:
//   picker_sheet_name(TileSheet::SHEET_DECOR0);   // -> "Decor0"
std::string picker_sheet_name(TileSheet sheet)
{
	for (size_t sheetIndex = 0; sheetIndex < std::size(PICKER_SHEETS); ++sheetIndex)
	{
		if (PICKER_SHEETS[sheetIndex] == sheet)
		{
			return std::string(PICKER_SHEET_NAMES[sheetIndex]);
		}
	}
	return std::to_string(static_cast<int>(sheet));
}

// Layout (all in tiles; pixels = value * tile_size)
constexpr int LEFT_W_TILES = 11;
constexpr int RIGHT_W_TILES = 13;
constexpr int TOP_H_TILES = 1;
constexpr int BOT_H_TILES = 1;

// The guide box's lines below its "HOW IT WORKS" heading; the box's height is derived from their count.
constexpr const char* GUIDE[] = {
	"[N]      new canvas",
	"[Ctrl+S] name + save",
	"[Enter]  load selected",
	"[Del]    delete selected",
	"Prefabs stamp decor",
	"onto BSP rooms at",
	"matching depth + size."
};

// The prefab list's rows in the right panel at the renderer's current screen, tile and font size.
PrefabListRows prefab_list_rows(const Renderer& renderer)
{
	const int tileSize = renderer.get_tile_size();
	const int panelY = TOP_H_TILES * tileSize;
	const int panelH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;
	return PrefabListRows{ panelY, panelH, renderer.get_font_size() };
}

// The canvas's cells at the renderer's current tile size, with cell (0, 0) moved by the pan.
CanvasGrid canvas_grid(const Renderer& renderer, int panX, int panY)
{
	const int tileSize = renderer.get_tile_size();
	return CanvasGrid{ LEFT_W_TILES * tileSize - panX, TOP_H_TILES * tileSize - panY, tileSize };
}
} // namespace

// ---------------------------------------------------------------------------
// Prefab list rows
// ---------------------------------------------------------------------------

PrefabListRows::PrefabListRows(int panelTop, int panelHeight, int fontSize)
	: rowHeight{ fontSize + 4 }
{
	// The "SAVED PREFABS" header takes one row and a 4-pixel gap above the first prefab.
	const int headerHeight = rowHeight + 4;
	top = panelTop + headerHeight;
	// The guide box holds the heading plus one row per guide line, at the list's pitch, plus 8 pixels.
	const int guideRows = static_cast<int>(std::size(GUIDE)) + 1;
	guideTop = panelTop + panelHeight - (guideRows * rowHeight + 8);
	visibleRows = whole_rows_within(guideTop - top, rowHeight);
}

int PrefabListRows::guide_top() const
{
	return guideTop;
}

int PrefabListRows::row_top(size_t slot) const
{
	return top + static_cast<int>(slot) * rowHeight;
}

int PrefabListRows::row_height() const
{
	return rowHeight;
}

size_t PrefabListRows::visible_rows() const
{
	return visibleRows;
}

std::optional<size_t> PrefabListRows::slot_at(int pixelY) const
{
	// whole_rows_within counts a negative offset as 0 rows, which would make the header select the first row.
	if (pixelY < top)
	{
		return std::nullopt;
	}
	const size_t slot = whole_rows_within(pixelY - top, rowHeight);
	if (slot >= visibleRows)
	{
		return std::nullopt;
	}
	return slot;
}

// ---------------------------------------------------------------------------
// Canvas grid
// ---------------------------------------------------------------------------

std::optional<int> parse_typed_size(std::string_view typed)
{
	int value{ 0 };
	const char* const end = typed.data() + typed.size();
	const auto [stoppedAt, error] = std::from_chars(typed.data(), end, value);

	// Only the whole text read as one number counts; a prefix of it does not.
	if (error != std::errc{} || stoppedAt != end)
	{
		return std::nullopt;
	}
	return value;
}

CanvasGrid::CanvasGrid(int gridOriginX, int gridOriginY, int cellSize)
	: originX{ gridOriginX },
	  originY{ gridOriginY },
	  tileSize{ cellSize }
{
	assert(tileSize > 0 && "CanvasGrid: a cell has no size");
}

int CanvasGrid::cell_index(int offset) const
{
	// Division truncates toward zero, which would put the strip just left of the origin in cell 0.
	int index = offset / tileSize;
	if (offset % tileSize < 0)
	{
		--index;
	}
	return index;
}

int CanvasGrid::cell_left(int col) const
{
	return originX + col * tileSize;
}

int CanvasGrid::cell_top(int row) const
{
	return originY + row * tileSize;
}

int CanvasGrid::column_at(int pixelX) const
{
	return cell_index(pixelX - originX);
}

int CanvasGrid::row_at(int pixelY) const
{
	return cell_index(pixelY - originY);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RoomEditor::enter(PrefabLibrary& lib, Palette& editedPalette)
{
	library = &lib;
	palette = &editedPalette;
	active = true;
	mode = EditorMode::NORMAL;
	inputBuffer.clear();
	listSelected.reset();
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
	// Every input and render path below reads the palette that enter() stored.
	assert(palette && "RoomEditor::tick: called before enter");
	tileConfig = ctx.tileConfig;
	handle_input(ctx);

	render(ctx);
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

	Prefab prefab;
	prefab.name = name.empty() ? std::format("prefab_{}x{}", canvasWidth, canvasHeight) : name;
	prefab.depthMin = depthMin;
	prefab.depthMax = depthMax;
	prefab.weight = weight;

	// Merge base + decor layers: decor symbol wins when present.
	prefab.rows.clear();
	prefab.rows.reserve(canvasHeight);
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
		prefab.rows.push_back(std::move(rowStr));
	}

	prefabName = prefab.name;
	library->add_or_replace(std::move(prefab));
	if (save_library() == SaveOutcome::SAVED)
	{
		set_status(std::format("Saved: {}", prefabName));
	}
}

void RoomEditor::do_load(size_t prefabIndex)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefabIndex >= all.size())
	{
		return;
	}

	const Prefab& prefab = all[prefabIndex];
	prefabName = prefab.name;
	depthMin = prefab.depthMin;
	depthMax = prefab.depthMax;
	weight = prefab.weight;

	canvasHeight = static_cast<int>(prefab.rows.size());
	canvasWidth = canvasHeight > 0 ? static_cast<int>(prefab.rows[0].size()) : 0;
	canvas.assign(canvasHeight, std::string(canvasWidth, '.'));
	decorCanvas.assign(canvasHeight, std::string(canvasWidth, ' '));

	auto is_structural_sym = [](char symbol) -> bool
	{
		return symbol == '#' || symbol == '.' || symbol == ',' || symbol == '+' || symbol == '~';
	};

	for (int row = 0; row < canvasHeight; ++row)
	{
		const std::string& srcRow = prefab.rows[row];
		for (int col = 0; col < static_cast<int>(srcRow.size()) && col < canvasWidth; ++col)
		{
			char symbol = srcRow[col];
			if (is_structural_sym(symbol))
			{
				canvas[row][col] = symbol;
				decorCanvas[row][col] = ' ';
			}
			else
			{
				canvas[row][col] = '.';
				decorCanvas[row][col] = symbol;
			}
		}
	}

	panX = 0;
	panY = 0;
	set_status(std::format("Loaded: {}", prefabName));
}

void RoomEditor::do_delete(size_t prefabIndex)
{
	if (!library)
	{
		return;
	}
	const auto& all = library->all();
	if (prefabIndex >= all.size())
	{
		return;
	}

	std::string name = all[prefabIndex].name;
	library->remove(name);
	listSelected.reset();
	if (save_library() == SaveOutcome::SAVED)
	{
		set_status(std::format("Deleted: {}", name));
	}
}

void RoomEditor::set_status(const std::string& message)
{
	statusMsg = message;
	statusTime = GetTime();
}

RoomEditor::SaveOutcome RoomEditor::save_library()
{
	assert(library && "RoomEditor::save_library: called before enter");
	try
	{
		library->save(Paths::PREFABS);
	}
	catch (const std::runtime_error& refusal)
	{
		set_status(std::format("SAVE FAILED: {}", refusal.what()));
		return SaveOutcome::FAILED;
	}
	return SaveOutcome::SAVED;
}

RoomEditor::SaveOutcome RoomEditor::save_palette()
{
	assert(palette && "RoomEditor::save_palette: called before enter");
	try
	{
		palette->save();
	}
	catch (const std::runtime_error& refusal)
	{
		set_status(std::format("SAVE FAILED: {}", refusal.what()));
		return SaveOutcome::FAILED;
	}
	return SaveOutcome::SAVED;
}

void RoomEditor::commit_symbol_label(char symbol, std::string_view label)
{
	assert(palette && "RoomEditor::commit_symbol_label: called before enter");
	const PaletteEdit edit = palette->set_symbol_label(symbol, label);
	if (edit != PaletteEdit::APPLIED)
	{
		set_status(std::format("NOT SAVED: {}", palette_edit_refusal(edit)));
		return;
	}
	if (save_palette() == SaveOutcome::SAVED)
	{
		set_status(std::format("Label: '{}'={}", symbol, label));
	}
}

void RoomEditor::commit_symbol_tile(char symbol, TileRef tile)
{
	assert(palette && "RoomEditor::commit_symbol_tile: called before enter");
	const PaletteEdit edit = palette->set_symbol_tile(symbol, tile);
	if (edit != PaletteEdit::APPLIED)
	{
		set_status(std::format("NOT SAVED: {}", palette_edit_refusal(edit)));
		return;
	}
	if (save_palette() == SaveOutcome::SAVED)
	{
		set_status(std::format("'{}' -> tile ({},{}) sheet {}", symbol, tile.col, tile.row, picker_sheet_name(tile.sheet)));
	}
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
		handle_input_text();
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
	if (IsKeyPressed(KEY_F3))
	{
		const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
		if (paletteIndex < symbolRows.size())
		{
			inputBuffer = symbolRows[paletteIndex].label;
			mode = EditorMode::INPUT_LABEL;
		}
		return;
	}

	// Palette navigation. The rows always begin with the structural symbols, so both wraps have a last entry.
	static_assert(!STRUCTURAL_SYMBOLS.empty(), "RoomEditor: the palette's wrapping navigation needs a row");
	const std::vector<SymbolRow> symbolRows = palette->symbol_rows();

	if (IsKeyPressed(KEY_UP))
	{
		paletteIndex = paletteIndex == 0 ? symbolRows.size() - 1 : paletteIndex - 1;
	}
	if (IsKeyPressed(KEY_DOWN))
	{
		paletteIndex = (paletteIndex + 1) % symbolRows.size();
	}

	// New canvas
	if (IsKeyPressed(KEY_N))
	{
		mode = EditorMode::INPUT_WIDTH;
		inputBuffer.clear();
		return;
	}

	// Delete selected prefab
	if (IsKeyPressed(KEY_DELETE) && listSelected.has_value())
	{
		do_delete(*listSelected);
		return;
	}

	// Load selected prefab
	if (IsKeyPressed(KEY_ENTER) && listSelected.has_value())
	{
		do_load(*listSelected);
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
			::Vector2 mousePosition = GetMousePosition();
			panMouseStartX = mousePosition.x;
			panMouseStartY = mousePosition.y;
			panStartX = panX;
			panStartY = panY;
		}
		else
		{
			::Vector2 mousePosition = GetMousePosition();
			panX = panStartX - static_cast<int>(mousePosition.x - panMouseStartX);
			panY = panStartY - static_cast<int>(mousePosition.y - panMouseStartY);
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
		::Vector2 mousePosition = GetMousePosition();
		int delta = (wheel > 0.0f) ? -1 : 1;

		auto is_mouse_over_left_panel = [this, &renderer, tileSize, &mousePosition]()
		{
			return screen_to_palette_index(renderer, static_cast<int>(mousePosition.x), static_cast<int>(mousePosition.y)).has_value() ||
				static_cast<int>(mousePosition.x) < LEFT_W_TILES * tileSize;
		};

		if (is_mouse_over_left_panel())
		{
			const size_t paletteSize = palette->symbol_rows().size();
			paletteIndex = step_within(paletteIndex, delta, paletteSize - 1);
		}
		else
		{
			const size_t count = library->all().size();
			const size_t lastPrefab = count == 0 ? 0 : count - 1;
			// A delete can leave the scroll past the shortened list; any notch then brings it back to the last row.
			listScroll = listScroll > lastPrefab ? lastPrefab : step_within(listScroll, delta, lastPrefab);
		}
	}

	// Left click -- paint on canvas or select palette / prefab list entry
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		::Vector2 mousePosition = GetMousePosition();

		// Left panel -- select palette symbol
		auto palIdx = screen_to_palette_index(renderer, static_cast<int>(mousePosition.x), static_cast<int>(mousePosition.y));
		if (palIdx)
		{
			paletteIndex = *palIdx;
			return;
		}

		// Canvas
		if (const std::optional<CanvasCell> cell = screen_to_canvas(renderer, static_cast<int>(mousePosition.x), static_cast<int>(mousePosition.y)))
		{
			char symbol = selected_sym();
			bool structural = (symbol == '#' || symbol == '.' || symbol == ',' ||
				symbol == '+' || symbol == '~');
			if (structural)
			{
				canvas[cell->row][cell->col] = symbol;
				decorCanvas[cell->row][cell->col] = ' '; // clear any decor overlay
			}
			else
			{
				decorCanvas[cell->row][cell->col] = symbol; // layer over the base tile
			}
			return;
		}

		// Right panel list
		auto listIdx = screen_to_list_index(renderer, static_cast<int>(mousePosition.x), static_cast<int>(mousePosition.y));
		if (listIdx)
		{
			listSelected = *listIdx;
			return;
		}
	}

	// Right click -- erase on canvas
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		::Vector2 mousePosition = GetMousePosition();
		if (const std::optional<CanvasCell> cell = screen_to_canvas(renderer, static_cast<int>(mousePosition.x), static_cast<int>(mousePosition.y)))
		{
			// First erase decor overlay; if already empty, erase the base tile.
			if (decorCanvas[cell->row][cell->col] != ' ')
			{
				decorCanvas[cell->row][cell->col] = ' ';
			}
			else
			{
				bool onBorder = (cell->col == 0 || cell->col == canvasWidth - 1 || cell->row == 0 || cell->row == canvasHeight - 1);
				canvas[cell->row][cell->col] = onBorder ? '#' : '.';
			}
		}
	}
}

void RoomEditor::handle_input_text()
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
			const std::optional<int> typedWidth = parse_typed_size(inputBuffer);
			if (!typedWidth)
			{
				set_status(std::format("Width '{}' is not a number -- kept {}", inputBuffer, canvasWidth));
			}
			pendingWidth = std::clamp(typedWidth.value_or(canvasWidth), 3, 60);
			mode = EditorMode::INPUT_HEIGHT;
			inputBuffer.clear();
		}
		else if (mode == EditorMode::INPUT_HEIGHT)
		{
			const std::optional<int> typedHeight = parse_typed_size(inputBuffer);
			if (!typedHeight)
			{
				set_status(std::format("Height '{}' is not a number -- kept {}", inputBuffer, canvasHeight));
			}
			new_canvas(pendingWidth, std::clamp(typedHeight.value_or(canvasHeight), 3, 40));
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
			if (!inputBuffer.empty())
			{
				const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
				if (paletteIndex < symbolRows.size())
				{
					commit_symbol_label(symbolRows[paletteIndex].symbol, inputBuffer);
				}
			}
			mode = EditorMode::NORMAL;
			inputBuffer.clear();
		}
		return;
	}

	// Character input (digits for size modes, any printable for name)
	int pressed = GetCharPressed();
	while (pressed > 0)
	{
		bool accept = false;
		if (mode == EditorMode::INPUT_WIDTH || mode == EditorMode::INPUT_HEIGHT)
		{
			accept = (pressed >= '0' && pressed <= '9') && inputBuffer.size() < 3;
		}
		else
		{
			accept = (pressed >= 32 && pressed <= 126) && inputBuffer.size() < 40;
		}
		// INPUT_LABEL uses the same printable-character rule as INPUT_NAME

		if (accept)
		{
			inputBuffer += static_cast<char>(pressed);
		}

		pressed = GetCharPressed();
	}
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

// Returns the canvas cell under a screen pixel, nullopt if outside the canvas.
std::optional<RoomEditor::CanvasCell> RoomEditor::screen_to_canvas(
	const Renderer& renderer,
	int mousePx,
	int mousePy) const
{
	int tileSize = renderer.get_tile_size();
	int areaX = LEFT_W_TILES * tileSize;
	int areaY = TOP_H_TILES * tileSize;
	int areaW = renderer.get_screen_width() - (LEFT_W_TILES + RIGHT_W_TILES) * tileSize;
	int areaH = renderer.get_screen_height() - (TOP_H_TILES + BOT_H_TILES) * tileSize;

	if (mousePx < areaX || mousePx >= areaX + areaW)
	{
		return std::nullopt;
	}
	if (mousePy < areaY || mousePy >= areaY + areaH)
	{
		return std::nullopt;
	}

	// The cells the draw places; a pixel left of or above cell (0, 0) is a negative cell and rejected below.
	const CanvasGrid grid = canvas_grid(renderer, panX, panY);
	const int col = grid.column_at(mousePx);
	const int row = grid.row_at(mousePy);

	if (col < 0 || col >= canvasWidth)
	{
		return std::nullopt;
	}
	if (row < 0 || row >= canvasHeight)
	{
		return std::nullopt;
	}

	return CanvasCell{ col, row };
}

// Returns prefab list index from screen pixel in right panel. Returns nullopt if outside panel.
std::optional<size_t> RoomEditor::screen_to_list_index(const Renderer& renderer, int mousePx, int mousePy) const
{
	if (!library)
	{
		return std::nullopt;
	}

	int tileSize = renderer.get_tile_size();
	int panelX = renderer.get_screen_width() - RIGHT_W_TILES * tileSize;

	if (mousePx < panelX || mousePx >= renderer.get_screen_width())
	{
		return std::nullopt;
	}

	// The rows the draw places; slot_at rejects the header and the strip below the last whole row.
	const std::optional<size_t> slot = prefab_list_rows(renderer).slot_at(mousePy);
	if (!slot)
	{
		return std::nullopt;
	}
	const size_t listIndex = listScroll + *slot;

	if (listIndex >= library->all().size())
	{
		return std::nullopt;
	}

	return listIndex;
}

// Returns palette index from screen pixel in left panel. Returns nullopt if outside panel.
std::optional<size_t> RoomEditor::screen_to_palette_index(const Renderer& renderer, int mousePx, int mousePy) const
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

	const size_t maxVisible = whole_rows_within(panelH, tileSize);
	const size_t slot = whole_rows_within(mousePy - panelY, tileSize);

	// The strip scrolls only to keep the selected entry on its last visible slot.
	const size_t scrollStart = paletteIndex >= maxVisible ? paletteIndex - maxVisible + 1 : 0;

	const size_t paletteEntryIndex = scrollStart + slot;
	if (paletteEntryIndex >= palette->symbol_rows().size() || slot >= maxVisible)
	{
		return std::nullopt;
	}

	return paletteEntryIndex;
}

// ---------------------------------------------------------------------------
// Symbol helpers
// ---------------------------------------------------------------------------

// Current selected symbol character.
char RoomEditor::selected_sym() const
{
	const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
	if (paletteIndex >= symbolRows.size())
	{
		return '.';
	}
	return symbolRows[paletteIndex].symbol;
}

// Tile to draw for a symbol in the canvas and palette.
TileRef RoomEditor::symbol_tile_id(char symbol) const
{
	if (!tileConfig)
	{
		return TileRef{};
	}
	const auto& tileConfigRef = *tileConfig;
	switch (symbol)
	{
	case '#':
	{
		return tileConfigRef.get("TILE_WALL_STONE");
	}
	case '.':
	{
		return tileConfigRef.get("TILE_FLOOR_STONE");
	}
	case ',':
	{
		return tileConfigRef.get("TILE_CORRIDOR");
	}
	case '+':
	{
		return tileConfigRef.get("TILE_DOOR_CLOSED");
	}
	case '~':
	{
		return tileConfigRef.get("TILE_WATER");
	}
	default:
	{
		return palette->resolve_decor(symbol);
	}
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
	char symbol = canvas[row][col];

	if (symbol == '#' && tileConfig)
	{
		return Autotile::wall_resolve_mask(tileConfig->get_wall_autotile("WALL_AUTOTILE_STONE"), canvas_wall_mask(col, row));
	}

	if (symbol == '.' && tileConfig)
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

	return symbol_tile_id(symbol);
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

	const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
	const size_t maxVisible = whole_rows_within(panelH, tileSize);

	// Scroll to keep selected entry visible
	const size_t scrollStart = paletteIndex >= maxVisible ? paletteIndex - maxVisible + 1 : 0;

	for (size_t slot = 0; slot < maxVisible && scrollStart + slot < symbolRows.size(); ++slot)
	{
		const size_t paletteEntryIndex = scrollStart + slot;
		int slotPixelY = panelY + static_cast<int>(slot) * tileSize;
		char symbol = symbolRows[paletteEntryIndex].symbol;

		bool selected = (paletteEntryIndex == paletteIndex);

		if (selected)
		{
			DrawRectangle(0, slotPixelY, panelW, tileSize, Color{ 60, 60, 100, 200 });
		}

		TileRef tile = symbol_tile_id(symbol);
		bool isStructural = (symbol == '#' || symbol == '.' || symbol == ',' ||
			symbol == '+' || symbol == '~');

		if (isStructural)
		{
			if (tile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ 2, slotPixelY }, tile);
			}
			else
			{
				Color block{};
				if (symbol == '#')
				{
					block = Color{ 90, 90, 100, 255 };
				}
				else if (symbol == '.')
				{
					block = Color{ 70, 55, 40, 255 };
				}
				else if (symbol == ',')
				{
					block = Color{ 55, 55, 60, 255 };
				}
				else if (symbol == '+')
				{
					block = Color{ 100, 70, 40, 255 };
				}
				else if (symbol == '~')
				{
					block = Color{ 30, 60, 120, 255 };
				}
				else
				{
					block = Color{ 50, 50, 50, 255 };
				}
				DrawRectangle(4, slotPixelY + 4, tileSize - 8, tileSize - 8, block);
			}
		}
		else
		{
			DrawRectangle(2, slotPixelY, tileSize, tileSize, Color{ 70, 58, 42, 255 });
			if (tile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ 2, slotPixelY }, tile);
			}
		}

		// Label
		Color labelCol = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 180, 180, 200, 200 };

		renderer.draw_text_color(Vector2D{ tileSize + 4, slotPixelY + (tileSize - renderer.get_font_size()) / 2 }, symbolRows[paletteEntryIndex].label, labelCol);

		if (selected)
		{
			DrawRectangleLines(1, slotPixelY + 1, panelW - 2, tileSize - 2, Color{ 200, 200, 100, 180 });
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

	const CanvasGrid grid = canvas_grid(renderer, panX, panY);
	int startCol = std::max(0, grid.column_at(areaX));
	int startRow = std::max(0, grid.row_at(areaY));
	int endCol = std::min(canvasWidth, startCol + areaW / tileSize + 2);
	int endRow = std::min(canvasHeight, startRow + areaH / tileSize + 2);

	static constexpr Color FLOOR_BG = { 70, 58, 42, 255 };
	static constexpr Color GRID_COLOR = { 80, 80, 110, 200 };

	for (int row = startRow; row < endRow; ++row)
	{
		for (int col = startCol; col < endCol; ++col)
		{
			int cellPixelX = grid.cell_left(col);
			int cellPixelY = grid.cell_top(row);

			// Base layer (structural)
			TileRef baseTile = canvas_tile_id(col, row);
			if (baseTile.is_valid())
			{
				renderer.draw_tile_screen(Vector2D{ cellPixelX, cellPixelY }, baseTile);
			}
			else
			{
				DrawRectangle(cellPixelX, cellPixelY, tileSize, tileSize, FLOOR_BG);
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
					renderer.draw_tile_screen(Vector2D{ cellPixelX, cellPixelY }, decorTile);
				}
			}

			DrawRectangleLines(cellPixelX, cellPixelY, tileSize, tileSize, GRID_COLOR);
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

	const PrefabListRows rows = prefab_list_rows(renderer);
	const auto& all = library->all();

	for (size_t slot = 0; slot < rows.visible_rows() && listScroll + slot < all.size(); ++slot)
	{
		const size_t listIndex = listScroll + slot;
		int rowPixelY = rows.row_top(slot);

		bool selected = (listSelected == listIndex);

		if (selected)
		{
			DrawRectangle(panelX, rowPixelY, panelW, rows.row_height(), Color{ 50, 50, 90, 200 });
		}

		const Prefab& prefab = all[listIndex];
		std::string label = std::format("{}  {}x{}", prefab.name, prefab.width(), prefab.height());

		Color entryCol = selected
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 200, 200 };

		renderer.draw_text_color(Vector2D{ panelX + 6, rowPixelY + 2 }, label, entryCol);
	}

	// Empty state
	if (all.empty())
	{
		renderer.draw_text_color(
			Vector2D{ panelX + 6, rows.row_top(0) + 4 }, "No prefabs yet.", Color{ 100, 100, 120, 150 });
	}

	// Workflow guidance -- drawn in the bottom portion of the right panel
	int fontH = renderer.get_font_size() + 4;
	int guideY = rows.guide_top();
	int guideH = panelY + panelH - guideY;

	DrawLine(panelX, guideY, panelX + panelW, guideY, Color{ 60, 60, 100, 200 });
	DrawRectangle(panelX, guideY + 1, panelW, guideH - 1, Color{ 10, 10, 18, 200 });

	renderer.draw_text_color(Vector2D{ panelX + 6, guideY + 4 }, "HOW IT WORKS", Color{ 140, 140, 220, 255 });

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

	if (IsKeyPressed(KEY_ENTER))
	{
		const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
		if (paletteIndex < symbolRows.size())
		{
			commit_symbol_tile(symbolRows[paletteIndex].symbol, TileRef{ sheetId, pickerCol, pickerRow });
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
			int cellPixelX = offX + col * PICK_TS;
			int cellPixelY = offY + row * PICK_TS;
			TileRef tileRef{ sheetId, col, row };

			// Dark bg so decor tiles are visible
			DrawRectangle(cellPixelX, cellPixelY, PICK_TS, PICK_TS, Color{ 20, 16, 12, 255 });

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tileRef, PICK_TS);

			// Selected cell highlight
			if (col == pickerCol && row == pickerRow)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, PICK_TS, PICK_TS, Color{ 255, 230, 50, 255 });
			}
		}
	}

	// Info bar at top
	const std::vector<SymbolRow> symbolRows = palette->symbol_rows();
	std::string symInfo = "?";
	if (paletteIndex < symbolRows.size())
	{
		char symbol = symbolRows[paletteIndex].symbol;
		symInfo = std::format("Assigning tile for '{}' ({})",
			symbol,
			palette->symbol_label(symbol));
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
