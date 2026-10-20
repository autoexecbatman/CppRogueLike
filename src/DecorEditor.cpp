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

	if (paletteIndex >= static_cast<int>(palette.size()))
	{
		paletteIndex = std::max(0, static_cast<int>(palette.size()) - 1);
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
	lastSaveTime = GetTime();
	std::clog << std::format("[DecorEditor] palette saved: {}\n", abs.string());
}

void DecorEditor::load_palette(std::string_view path)
{
	palettePath = std::string(path);
	palette.clear();
	paletteIndex = 0;

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

	paletteIndex = (paletteIndex + 1) % static_cast<int>(palette.size());
}

void DecorEditor::cycle_prev()
{
	if (palette.empty())
	{
		return;
	}

	int paletteSize = static_cast<int>(palette.size());
	paletteIndex = (paletteIndex - 1 + paletteSize) % paletteSize;
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
			save_palette(palettePath);
		}
	}

	if (isBrowserOpen)
	{
		update_browser(renderer, palettePath);
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
	const int paletteSize = static_cast<int>(palette.size());

	constexpr int VISIBLE = 9;
	int half = VISIBLE / 2;

	DrawRectangle(0, 0, screenWidth, tileSize + 4, Color{ 0, 0, 0, 200 });

	for (int slot = 0; slot < VISIBLE; ++slot)
	{
		int idx = paletteIndex - half + slot;
		if (idx < 0 || idx >= paletteSize)
		{
			continue;
		}

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

	std::string info = std::format(
		"DECOR EDITOR  |  {}  |  Map: {}  |  Pos: {},{}"
		"  |  Tab browser  , / . cycle  LClick place  RClick erase  Ctrl+S save{}",
		tileName,
		activeKey.empty() ? "none" : activeKey,
		worldPosition.x,
		worldPosition.y,
		isSaveRecent ? "  -- SAVED!" : "");

	Color textColor = isSaveRecent
		? Color{ 100, 255, 100, 255 }
		: Color{ 255, 255, 180, 255 };

	renderer.draw_text_color(Vector2D{ 4, barY + 2 }, info, textColor);
}

// ---------------------------------------------------------------------------
// Sheet browser
// ---------------------------------------------------------------------------

void DecorEditor::update_browser(const Renderer& renderer, std::string_view sourcePath)
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
	renderer.draw_text_color(Vector2D{ PAD, 28 }, hintLine, Color{ 160, 160, 120, 255 });

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
	const int visibleRows = listVisibleHeight / LIST_ITEM_H;
	const int maxScroll = std::max(0, static_cast<int>(palette.size()) - visibleRows);
	listScroll = std::min(listScroll, maxScroll);

	if (isMouseInList)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			listScroll = std::clamp(listScroll - static_cast<int>(wheel), 0, maxScroll);
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
			listCursor = -1;
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

	for (int entryIndex = listScroll; entryIndex < static_cast<int>(palette.size()); ++entryIndex)
	{
		int itemY = listTop + (entryIndex - listScroll) * LIST_ITEM_H;
		if (itemY >= listTop + listVisibleHeight)
		{
			break;
		}

		bool isSelected = (entryIndex == listCursor);
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
				save_palette(sourcePath);
				listCursor = -1;
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
		save_palette(sourcePath);
		editing = false;
		browserSelected = TileRef{};
		listCursor = -1;
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
		Color spriteColor = listCursor >= 0
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
			listCursor = -1;
			isLabelAllSelected = false;
		}

		if (IsKeyPressed(KEY_ENTER) && !labelBuffer.empty())
		{
			palette_add_or_update(browserSelected, labelBuffer, symbolBuffer);
			if (auto found = palette_find(browserSelected))
			{
				listCursor = *found;
				listScroll = std::max(0, listCursor - visibleRows + 1);
			}
			save_palette(sourcePath);
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
