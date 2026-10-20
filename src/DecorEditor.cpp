// file: DecorEditor.cpp
#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <raylib.h>

#include "DecorEditor.h"
#include "ListCursor.h"
#include "Paths.h"
#include "Renderer.h"
#include "TileRefJson.h"

using json = nlohmann::json;

namespace
{
// The colour of a refused save, in the info bar and the browser header alike.
constexpr Color SAVE_FAILED_COLOR{ 255, 90, 90, 255 };
} // namespace

// ---------------------------------------------------------------------------

DecorEditor::DecorEditor() = default;

// ---------------------------------------------------------------------------
// Palette helpers
// ---------------------------------------------------------------------------

std::optional<size_t> DecorEditor::palette_find(TileRef tile) const
{
	for (size_t entryIndex = 0; entryIndex < palette.size(); ++entryIndex)
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

	paletteIndex = palette.empty() ? 0 : std::min(paletteIndex, palette.size() - 1);
}

// ---------------------------------------------------------------------------
// nlohmann serialization for PaletteEntry
// The tile goes through TileRefJson; an empty symbol string means no symbol.
// ---------------------------------------------------------------------------

void to_json(json& output, const DecorEditor::PaletteEntry& entry)
{
	output = TileRefJson::encode_tile(entry.tile);
	output["label"] = entry.label;
	output["symbol"] = entry.symbol ? std::string(1, entry.symbol) : std::string{};
}

void from_json(const json& source, DecorEditor::PaletteEntry& entry)
{
	entry.tile = TileRefJson::parse_tile(source);
	entry.label = source.at("label").get<std::string>();
	const std::string symbolString = source.at("symbol").get<std::string>();
	entry.symbol = symbolString.empty() ? 0 : symbolString[0];
}

// ---------------------------------------------------------------------------
// Persistence -- palette
// ---------------------------------------------------------------------------

void DecorEditor::save_palette(std::string_view path) const
{
	const auto resolved = Paths::resolve(path);

	// The rest of the file belongs to TileConfig, so a file that cannot be read back whole is not written.
	std::ifstream existingFile(resolved);
	if (!existingFile.is_open())
	{
		throw std::runtime_error(std::format(
			"DecorEditor::save_palette -- cannot open '{}' to keep its other sections",
			resolved.string()));
	}
	json document;
	try
	{
		document = json::parse(existingFile);
	}
	catch (const json::parse_error& error)
	{
		throw std::runtime_error(std::format(
			"DecorEditor::save_palette -- '{}' does not parse: {}",
			resolved.string(),
			error.what()));
	}
	existingFile.close();

	document["palette"] = palette;

	std::ofstream out(resolved);
	if (!out.is_open())
	{
		throw std::runtime_error(std::format(
			"DecorEditor::save_palette -- cannot open '{}' for writing",
			resolved.string()));
	}
	out << document.dump(4);
	out.flush();
	if (out.fail())
	{
		throw std::runtime_error(std::format(
			"DecorEditor::save_palette -- write failed for '{}'",
			resolved.string()));
	}
	std::clog << std::format("[DecorEditor] palette saved: {}\n", resolved.string());
}

void DecorEditor::save_palette_reporting()
{
	try
	{
		save_palette(palettePath);
	}
	catch (const std::runtime_error& refusal)
	{
		std::clog << std::format("[DecorEditor] {}\n", refusal.what());
		saveError = refusal.what();
		return;
	}

	saveError.reset();
	lastSaveTime = GetTime();
}

void DecorEditor::load_palette(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream paletteFile(resolved);
	if (!paletteFile.is_open())
	{
		throw std::runtime_error(std::format(
			"DecorEditor::load_palette -- cannot open '{}' -- the palette is required",
			resolved.string()));
	}

	// Read into a local, so a refused file leaves the palette that was loaded before it.
	const json document = json::parse(paletteFile);
	std::vector<PaletteEntry> loaded = document.at("palette").get<std::vector<PaletteEntry>>();
	const auto is_unlabelled = [](const PaletteEntry& entry)
	{
		return entry.label.empty();
	};
	std::erase_if(loaded, is_unlabelled);

	palettePath = std::string(path);
	palette = std::move(loaded);
	paletteIndex = 0;
	std::clog << std::format("[DecorEditor] palette loaded: {}\n", resolved.string());
}

// ---------------------------------------------------------------------------

void DecorEditor::set_active_map(long seed, int dungeonLevel)
{
	activeKey = std::format("{}_{}", seed, dungeonLevel);
}

void DecorEditor::toggle()
{
	active = !active;
	if (!active)
	{
		isBrowserOpen = false;
		editing = false;
	}
}

void DecorEditor::cycle_next()
{
	if (palette.empty())
	{
		return;
	}

	paletteIndex = (paletteIndex + 1) % palette.size();
}

void DecorEditor::cycle_prev()
{
	if (palette.empty())
	{
		return;
	}

	paletteIndex = paletteIndex == 0 ? palette.size() - 1 : paletteIndex - 1;
}

void DecorEditor::place(Vector2D worldPosition)
{
	if (palette.empty())
	{
		return;
	}

	current_map()[make_key(worldPosition)] = palette[paletteIndex].tile;
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
	return allOverrides[activeKey];
}

const std::unordered_map<uint32_t, TileRef>& DecorEditor::current_map() const
{
	if (allOverrides.contains(activeKey))
	{
		return allOverrides.at(activeKey);
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
		isBrowserOpen = !isBrowserOpen;
		if (!isBrowserOpen)
		{
			editing = false;
			save_palette_reporting();
		}
	}

	if (isBrowserOpen)
	{
		update_browser(renderer);
		return;
	}

	// Normal editor overlay
	const int tileSize = renderer.get_tile_size();
	const int cameraX = renderer.get_camera_x();
	const int cameraY = renderer.get_camera_y();

	::Vector2 mousePosition = GetMousePosition();
	const Vector2D worldPosition{
		(static_cast<int>(mousePosition.x) + cameraX) / tileSize,
		(static_cast<int>(mousePosition.y) + cameraY) / tileSize
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

	const int tileSize = renderer.get_tile_size();
	const int cameraX = renderer.get_camera_x();
	const int cameraY = renderer.get_camera_y();

	int pixelX = worldPosition.x * tileSize - cameraX;
	int pixelY = worldPosition.y * tileSize - cameraY;

	DrawRectangle(pixelX, pixelY, tileSize, tileSize, Color{ 255, 255, 0, 60 });
	DrawRectangleLines(pixelX, pixelY, tileSize, tileSize, Color{ 255, 255, 0, 220 });

	renderer.draw_tile(Vector2D{ worldPosition.x, worldPosition.y }, palette[paletteIndex].tile, Color{ 255, 255, 255, 180 });
}

void DecorEditor::draw_palette_strip(const Renderer& renderer) const
{
	if (palette.empty())
	{
		return;
	}

	const int tileSize = renderer.get_tile_size();
	const int screenWidth = renderer.get_screen_width();
	constexpr int VISIBLE = 9;
	constexpr size_t HALF = VISIBLE / 2;

	DrawRectangle(0, 0, screenWidth, tileSize + 4, Color{ 0, 0, 0, 200 });

	for (int slot = 0; slot < VISIBLE; ++slot)
	{
		// The slot shows the entry HALF places either side of the current one; past either end it shows nothing.
		const size_t offset = static_cast<size_t>(slot);
		if (paletteIndex + offset < HALF || paletteIndex + offset - HALF >= palette.size())
		{
			continue;
		}
		const size_t idx = paletteIndex + offset - HALF;

		int slotPixelX = slot * tileSize + (screenWidth - VISIBLE * tileSize) / 2;
		int slotPixelY = 2;

		bool isCurrent = (idx == paletteIndex);

		if (isCurrent)
		{
			DrawRectangle(slotPixelX, slotPixelY, tileSize, tileSize, Color{ 255, 255, 0, 80 });
		}

		renderer.draw_tile_screen(Vector2D{ slotPixelX, slotPixelY }, palette[idx].tile);

		if (isCurrent)
		{
			DrawRectangleLines(slotPixelX, slotPixelY, tileSize, tileSize, Color{ 255, 255, 0, 255 });
		}
	}
}

void DecorEditor::draw_info_bar(const Renderer& renderer, Vector2D worldPosition) const
{
	const int tileSize = renderer.get_tile_size();
	const int screenWidth = renderer.get_screen_width();

	int barY = tileSize + 6;
	DrawRectangle(0, barY, screenWidth, 20, Color{ 0, 0, 0, 180 });

	bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;

	std::string tileName = palette.empty()
		? "(no tiles -- Tab to open browser)"
		: palette[paletteIndex].label;

	// A refused save outranks a recent success: it stays until a save succeeds.
	std::string saveNote{};
	Color textColor{ 255, 255, 180, 255 };
	if (saveError)
	{
		saveNote = std::format("  -- SAVE FAILED: {}", *saveError);
		textColor = SAVE_FAILED_COLOR;
	}
	else if (isSaveRecent)
	{
		saveNote = "  -- SAVED!";
		textColor = Color{ 100, 255, 100, 255 };
	}

	std::string info = std::format(
		"DECOR EDITOR  |  {}  |  Map: {}  |  Pos: {},{}"
		"  |  Tab browser  , / . cycle  LClick place  RClick erase  Ctrl+S save{}",
		tileName,
		activeKey.empty() ? "none" : activeKey,
		worldPosition.x,
		worldPosition.y,
		saveNote);

	renderer.draw_text_color(Vector2D{ 4, barY + 2 }, info, textColor);
}

// ---------------------------------------------------------------------------
// Sheet browser
// ---------------------------------------------------------------------------

void DecorEditor::update_browser(const Renderer& renderer)
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
	const int totalSheets = renderer.get_loaded_sheet_count();
	auto advance_sheet = [&](int dir)
	{
		for (int sheetsTried = 0; sheetsTried < totalSheets; ++sheetsTried)
		{
			browserSheet = (browserSheet + dir + totalSheets) % totalSheets;
			if (renderer.sheet_is_loaded(static_cast<TileSheet>(browserSheet)))
			{
				break;
			}
		}
		browserScroll = 0;
		browserSelected = TileRef{};
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

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(browserSheet)))
	{
		advance_sheet(1);
	}

	const int sheetColumns = renderer.get_sheet_cols(static_cast<TileSheet>(browserSheet));
	const int sheetRows = renderer.get_sheet_rows(static_cast<TileSheet>(browserSheet));

	// --- Header (two lines) ---
	DrawRectangle(0, 0, screenWidth, HEADER_H, Color{ 20, 20, 40, 255 });

	std::string headerLine = std::format(
		"TILE BROWSER  |  {} ({}/{})  |  {}x{} tiles",
		renderer.get_sheet_name(static_cast<TileSheet>(browserSheet)),
		browserSheet + 1,
		totalSheets,
		sheetColumns,
		sheetRows);

	std::string hintLine = "Left/Right: sheet    Tab: close    Click list entry: rename    [X]: delete    Click tile: add/edit";

	renderer.draw_text_color(Vector2D{ PAD, 6 }, headerLine, Color{ 255, 255, 180, 255 });
	if (saveError)
	{
		renderer.draw_text_color(Vector2D{ PAD, 28 }, std::format("SAVE FAILED: {}", *saveError), SAVE_FAILED_COLOR);
	}
	else
	{
		renderer.draw_text_color(Vector2D{ PAD, 28 }, hintLine, Color{ 160, 160, 120, 255 });
	}

	const int gridY = HEADER_H + 2;
	const int gridHeight = screenHeight - gridY - (editing ? EDIT_H : 0) - 2;

	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	const bool isMouseInList = mouse.x < LIST_W && mouse.y >= gridY && mouse.y < gridY + gridHeight;
	const bool isMouseInGrid = mouse.x >= LIST_W;

	// --- Palette list panel ---
	DrawRectangle(0, gridY, LIST_W, gridHeight, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_W, gridY, LIST_W, gridY + gridHeight, Color{ 80, 80, 120, 255 });
	renderer.draw_text_color(Vector2D{ PAD, gridY + 4 }, "PALETTE", Color{ 200, 200, 100, 255 });

	const int listTop = gridY + 20;
	const int listVisibleHeight = gridHeight - 20;
	const size_t visibleRows = whole_rows_within(listVisibleHeight, LIST_ITEM_H);
	const size_t maxScroll = palette.size() > visibleRows ? palette.size() - visibleRows : 0;
	listScroll = std::min(listScroll, maxScroll);

	if (isMouseInList)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			listScroll = step_within(listScroll, -static_cast<int>(wheel), maxScroll);
		}
	}

	// Delete button column on the right edge of each list item.
	constexpr int DEL_W = 22;
	constexpr int CHAR_W = 8; // approx px per glyph (default raylib font)
	const int listTextX = PAD + LIST_TILE + 4;
	const int deleteButtonX = LIST_W - PAD - DEL_W;
	const int maxChars = std::max(3, (deleteButtonX - listTextX - 4) / CHAR_W);

	auto truncate = [&](std::string text) -> std::string
	{
		if (static_cast<int>(text.size()) > maxChars)
		{
			return text.substr(0, maxChars - 3) + "...";
		}
		return text;
	};

	// The one place the edit panel opens for any TileRef, with all editing state read from the palette.
	auto begin_edit = [this, &renderer](TileRef tile)
	{
		browserSelected = tile;
		auto foundIndex = palette_find(tile);
		if (foundIndex)
		{
			listCursor = *foundIndex;
			labelBuffer = palette[*foundIndex].label;
			symbolBuffer = palette[*foundIndex].symbol;
		}
		else
		{
			listCursor.reset();
			labelBuffer = std::format("{} {},{}",
				renderer.get_sheet_name(tile.sheet),
				tile.col,
				tile.row);
			symbolBuffer = 0;
		}
		editing = true;
		isEditingSymbol = false;
		isLabelAllSelected = true;
	};

	BeginScissorMode(0, listTop, LIST_W, listVisibleHeight);

	for (size_t entryIndex = listScroll; entryIndex < palette.size(); ++entryIndex)
	{
		int itemY = listTop + static_cast<int>(entryIndex - listScroll) * LIST_ITEM_H;
		if (itemY >= listTop + listVisibleHeight)
		{
			break;
		}

		bool isSelected = (listCursor == entryIndex);
		bool hovered = isMouseInList && mouse.y >= itemY && mouse.y < itemY + LIST_ITEM_H;

		if (isSelected)
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
		std::string entryText;
		Color textColor;
		if (isSelected && editing)
		{
			// Show what the user is currently typing, with cursor.
			std::string live = isLabelAllSelected
				? std::format("[{}]", labelBuffer) // selected-all state
				: std::format("{}|", labelBuffer); // normal typing cursor
			if (symbolBuffer)
			{
				live += std::format(" ({})", symbolBuffer);
			}
			entryText = truncate(live);
			textColor = Color{ 255, 220, 60, 255 };
		}
		else
		{
			entryText = palette[entryIndex].label;
			if (palette[entryIndex].symbol)
			{
				entryText += std::format(" [{}]", palette[entryIndex].symbol);
			}
			entryText = truncate(entryText);
			textColor = isSelected ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
		}
		renderer.draw_text_color(Vector2D{ listTextX, itemY + (LIST_ITEM_H - 16) / 2 }, entryText, textColor);

		// [X] delete button -- visible on selected or hovered row.
		if (isSelected || hovered)
		{
			int buttonY = itemY + 4;
			int buttonHeight = LIST_ITEM_H - 8;
			bool isDeleteButtonHovered = isMouseInList && mouse.x >= deleteButtonX && mouse.x < deleteButtonX + DEL_W && mouse.y >= itemY && mouse.y < itemY + LIST_ITEM_H;

			Color deleteButtonColor = isDeleteButtonHovered ? Color{ 220, 50, 50, 255 } : Color{ 140, 40, 40, 200 };
			DrawRectangle(deleteButtonX, buttonY, DEL_W, buttonHeight, deleteButtonColor);
			renderer.draw_text_color(Vector2D{ deleteButtonX + 5, itemY + (LIST_ITEM_H - 16) / 2 }, "X", Color{ 255, 255, 255, 255 });

			if (isDeleteButtonHovered && clicked)
			{
				palette_remove(palette[entryIndex].tile);
				save_palette_reporting();
				listCursor.reset();
				browserSelected = TileRef{};
				editing = false;
				break; // vector modified -- exit loop safely
			}
		}

		// Entry click (outside [X] button) -- navigate browser to tile's sheet then edit.
		if (hovered && clicked && mouse.x < deleteButtonX)
		{
			if (renderer.sheet_is_loaded(palette[entryIndex].tile.sheet))
			{
				browserSheet = static_cast<int>(palette[entryIndex].tile.sheet);
				browserScroll = std::max(0, palette[entryIndex].tile.row - 2);
			}
			begin_edit(palette[entryIndex].tile);
		}
	}
	EndScissorMode();

	// --- Tile grid (shifted right by LIST_W) ---
	if (isMouseInGrid)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			browserScroll -= static_cast<int>(wheel);
			browserScroll = std::max(0, std::min(browserScroll, sheetRows - 1));
		}
	}

	BeginScissorMode(LIST_W, gridY, screenWidth - LIST_W, gridHeight);

	for (int row = browserScroll; row < sheetRows; ++row)
	{
		int cellPixelY = gridY + (row - browserScroll) * (BROWSER_TILE + 2);
		if (cellPixelY >= gridY + gridHeight)
		{
			break;
		}

		for (int col = 0; col < sheetColumns; ++col)
		{
			int cellPixelX = LIST_W + PAD + col * (BROWSER_TILE + 2);
			TileRef tile{ static_cast<TileSheet>(browserSheet), col, row };

			bool isSelected = (tile == browserSelected);
			bool isInPalette = palette_find(tile).has_value();
			bool isTileHovered = (mouse.x >= cellPixelX && mouse.x < cellPixelX + BROWSER_TILE &&
				mouse.y >= cellPixelY && mouse.y < cellPixelY + BROWSER_TILE);

			if (isInPalette)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 20, 60, 20, 200 });
			}
			if (isSelected)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			}
			if (isTileHovered && !isSelected)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 30, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tile, BROWSER_TILE);

			if (isSelected)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
			}
			else if (isInPalette)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 220, 60, 200 });
			}

			if (isTileHovered)
			{
				auto foundIndex = palette_find(tile);
				if (foundIndex)
				{
					const auto& entry = palette[*foundIndex];
					std::string tip = entry.label;
					if (entry.symbol)
					{
						tip += std::format(" [{}]", entry.symbol);
					}
					int tipX = static_cast<int>(mouse.x) + 14;
					int tipY = static_cast<int>(mouse.y) - 18;
					DrawRectangle(tipX - 2, tipY - 2, static_cast<int>(tip.size()) * 8 + 6, 18, Color{ 0, 0, 0, 200 });
					renderer.draw_text_color(Vector2D{ tipX, tipY }, tip, Color{ 255, 255, 200, 255 });
				}
			}

			if (isTileHovered && clicked)
			{
				begin_edit(tile);
			}
		}
	}

	EndScissorMode();

	// Delete key
	if (browserSelected.is_valid() && IsKeyPressed(KEY_DELETE))
	{
		palette_remove(browserSelected);
		save_palette_reporting();
		editing = false;
		browserSelected = TileRef{};
		listCursor.reset();
	}

	// --- Edit panel ---
	if (editing)
	{
		int panelY = screenHeight - EDIT_H;
		DrawRectangle(0, panelY, screenWidth, EDIT_H, Color{ 10, 10, 30, 248 });
		DrawLine(0, panelY, screenWidth, panelY, Color{ 100, 100, 200, 255 });

		renderer.draw_tile_screen_sized(
			Vector2D{ PAD, panelY + (EDIT_H - BROWSER_TILE) / 2 },
			browserSelected,
			BROWSER_TILE);

		int textX = PAD + BROWSER_TILE + 12;

		// Sprite info (read-only -- set by clicking in browser)
		std::string spriteInfo = std::format(
			"Sprite: {} sheet:{} col:{} row:{}",
			renderer.get_sheet_name(browserSelected.sheet),
			static_cast<int>(browserSelected.sheet),
			browserSelected.col,
			browserSelected.row);
		Color spriteColor = listCursor.has_value()
			? Color{ 255, 180, 80, 255 }
			: Color{ 140, 200, 255, 255 };
		renderer.draw_text_color(Vector2D{ textX, panelY + 6 }, spriteInfo, spriteColor);

		// Label field -- highlighted when all-selected (next keystroke replaces)
		Color labelColor = !isEditingSymbol
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 80, 255 };
		std::string labelDisplay = isLabelAllSelected
			? std::format("Label: [{}]", labelBuffer) // brackets = all selected
			: std::format("Label: {}{}", labelBuffer, !isEditingSymbol ? "|" : "");
		renderer.draw_text_color(Vector2D{ textX, panelY + 26 }, labelDisplay, labelColor);

		// Symbol field
		Color symbolColor = isEditingSymbol
			? Color{ 255, 255, 100, 255 }
			: Color{ 160, 160, 80, 255 };
		renderer.draw_text_color(
			Vector2D{ textX, panelY + 46 }, std::format("Symbol: {}{}", symbolBuffer ? std::string(1, symbolBuffer) : "_", isEditingSymbol ? "|" : ""), symbolColor);

		renderer.draw_text_color(Vector2D{ textX, panelY + 72 }, "Tab=switch field  |  Enter=save  |  Esc=cancel  |  Del=remove", Color{ 140, 140, 140, 255 });

		if (IsKeyPressed(KEY_TAB))
		{
			isEditingSymbol = !isEditingSymbol;
			isLabelAllSelected = false;
		}

		if (IsKeyPressed(KEY_ESCAPE))
		{
			editing = false;
			browserSelected = TileRef{};
			listCursor.reset();
			isLabelAllSelected = false;
		}

		if (IsKeyPressed(KEY_ENTER) && !labelBuffer.empty())
		{
			palette_add_or_update(browserSelected, labelBuffer, symbolBuffer);
			if (auto found = palette_find(browserSelected))
			{
				listCursor = *found;
				listScroll = *listCursor + 1 > visibleRows ? *listCursor + 1 - visibleRows : 0;
			}
			save_palette_reporting();
			editing = false;
		}

		// The char InputSystem::poll() already took from GetCharPressed(); a second call here returns 0.
		const int pressed = bufferedCharacter;
		bufferedCharacter = 0;

		if (!isEditingSymbol)
		{
			if (IsKeyPressed(KEY_BACKSPACE))
			{
				labelBuffer.clear();
				isLabelAllSelected = false;
			}

			if (pressed >= 32 && pressed < 127)
			{
				if (isLabelAllSelected)
				{
					labelBuffer.clear();
					isLabelAllSelected = false;
				}
				labelBuffer += static_cast<char>(pressed);
			}
		}
		else
		{
			if (IsKeyPressed(KEY_BACKSPACE))
			{
				symbolBuffer = 0;
			}

			if (pressed > 32 && pressed < 127)
			{
				symbolBuffer = static_cast<char>(pressed);
			}
		}
	}
}
