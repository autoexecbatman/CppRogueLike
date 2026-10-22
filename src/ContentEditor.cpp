// file: ContentEditor.cpp
#include <algorithm>
#include <format>
#include <string>

#include <raylib.h>

#include "ContentEditor.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "ItemRegistry.h"
#include "MonsterRegistry.h"
#include "Paths.h"
#include "Renderer.h"

// ---------------------------------------------------------------------------
// Static monster table
// ---------------------------------------------------------------------------

namespace
{

struct MonsterEntry
{
	MonsterId id;
	const char* name;
};

constexpr MonsterEntry MONSTER_TABLE[] = {
	{ MonsterId::GOBLIN, "Goblin" },
	{ MonsterId::ORC, "Orc" },
	{ MonsterId::TROLL, "Troll" },
	{ MonsterId::DRAGON, "Dragon" },
	{ MonsterId::ARCHER, "Archer" },
	{ MonsterId::MAGE, "Mage" },
	{ MonsterId::WOLF, "Wolf" },
	{ MonsterId::FIRE_WOLF, "Fire Wolf" },
	{ MonsterId::ICE_WOLF, "Ice Wolf" },
	{ MonsterId::BAT, "Bat" },
	{ MonsterId::KOBOLD, "Kobold" },
	{ MonsterId::MIMIC, "Mimic" },
	{ MonsterId::SHOPKEEPER, "Shopkeeper" },
	{ MonsterId::SPIDER_SMALL, "Spider (Small)" },
	{ MonsterId::SPIDER_GIANT, "Spider (Giant)" },
	{ MonsterId::SPIDER_WEAVER, "Spider (Weaver)" },
};

// The height of one entry row. The keyboard and the draw both count rows with it.
constexpr int ITEM_H = 36;

} // namespace

// ---------------------------------------------------------------------------

void ContentEditor::toggle(ContentRegistry& registry, MonsterRegistry& monsters, const ItemRegistry& items)
{
	contentRegistry = &registry;

	if (active)
	{
		// Closing: persist all assignments immediately.
		ContentRegistryIO::save(*contentRegistry, Paths::CONTENT_TILES);
		monsters.save(Paths::MONSTERS);
	}

	active = !active;

	if (active && itemEntries.empty())
	{
		for (const std::string& key : items.get_all_keys())
		{
			const ItemParams& params = items.get_params(key);
			itemEntries.push_back({ std::string(params.name), key, 0 });
		}
		for (const auto& row : MONSTER_TABLE)
		{
			monsterEntries.push_back({ row.name, {}, static_cast<int>(row.id) });
		}
		listCursor.reset(active_entries().size());
	}
}

TileRef ContentEditor::current_tile(const MonsterRegistry& monsters) const
{
	const auto& entries = active_entries();
	if (entries.empty())
	{
		return TileRef{};
	}

	const auto& sel = entries[listCursor.current_row()];
	if (tab == 0)
	{
		if (!contentRegistry)
		{
			return TileRef{};
		}
		return contentRegistry->get_tile(sel.itemKey);
	}

	return monsters.get_tile(static_cast<MonsterId>(sel.entityKey));
}

void ContentEditor::assign_tile(TileRef tile, MonsterRegistry& monsters)
{
	auto& entries = active_entries();
	if (entries.empty())
	{
		return;
	}

	const auto& sel = entries[listCursor.current_row()];
	if (tab == 0)
	{
		if (contentRegistry)
		{
			contentRegistry->set_tile(sel.itemKey, tile);
		}
	}
	else
	{
		monsters.set_tile(static_cast<MonsterId>(sel.entityKey), tile);
	}
}

// ---------------------------------------------------------------------------
// update_and_render
// ---------------------------------------------------------------------------

void ContentEditor::update_and_render(const Renderer& renderer, ContentRegistry& registry, MonsterRegistry& monsters)
{
	if (!active)
	{
		return;
	}
	contentRegistry = &registry;

	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();

	constexpr int HEADER_H = 56;
	constexpr int HINT_H = 28;
	constexpr int LIST_W = 420;

	const int bodyY = HEADER_H;
	const int bodyHeight = screenHeight - HEADER_H - HINT_H;

	// The keys move within the rows draw_list draws, so both count them from bodyHeight.
	handle_keyboard(whole_rows_within(bodyHeight, ITEM_H));

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 220 });

	draw_header(renderer);

	draw_list(renderer, monsters, 0, bodyY, LIST_W, bodyHeight);
	draw_browser(renderer, monsters, LIST_W, bodyY, screenWidth - LIST_W, bodyHeight);
	draw_hint_bar(renderer);
}

void ContentEditor::draw_header(const Renderer& renderer)
{
	const int screenWidth = renderer.get_screen_width();
	constexpr int HEADER_H = 56;
	constexpr int TAB_W = 110;
	constexpr int TAB_H = 28;
	constexpr int TAB_Y = 20;

	DrawRectangle(0, 0, screenWidth, HEADER_H, Color{ 20, 20, 40, 255 });
	renderer.draw_text_color(Vector2D{ 8, 4 }, "CONTENT EDITOR", Color{ 255, 255, 180, 255 });

	const char* TAB_NAMES[2] = { "Items", "Monsters" };
	::Vector2 mouse = GetMousePosition();

	for (int tabIndex = 0; tabIndex < 2; ++tabIndex)
	{
		int tabPixelX = 8 + tabIndex * (TAB_W + 4);
		bool sel = (tab == tabIndex);
		Color bgColor = sel ? Color{ 80, 80, 0, 255 } : Color{ 30, 30, 60, 255 };
		Color textColor = sel ? Color{ 255, 255, 100, 255 } : Color{ 160, 160, 160, 255 };

		DrawRectangle(tabPixelX, TAB_Y, TAB_W, TAB_H, bgColor);
		DrawRectangleLines(tabPixelX, TAB_Y, TAB_W, TAB_H, Color{ 100, 100, 60, 255 });
		renderer.draw_text_color(Vector2D{ tabPixelX + 8, TAB_Y + 4 }, TAB_NAMES[tabIndex], textColor);

		bool isTabClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			mouse.x >= tabPixelX && mouse.x < tabPixelX + TAB_W &&
			mouse.y >= TAB_Y && mouse.y < TAB_Y + TAB_H;

		if (isTabClicked)
		{
			tab = tabIndex;
			listCursor.reset(active_entries().size());
		}
	}
}

void ContentEditor::draw_list(
	const Renderer& renderer,
	const MonsterRegistry& monsters,
	int listX,
	int listY,
	int listWidth,
	int listHeight)
{
	constexpr int TILE_SZ = 28;
	constexpr int PAD = 8;

	DrawRectangle(listX, listY, listWidth, listHeight, Color{ 10, 10, 20, 255 });
	DrawLine(listX + listWidth - 1, listY, listX + listWidth - 1, listY + listHeight, Color{ 80, 80, 120, 255 });

	const auto& entries = active_entries();
	const size_t visible = whole_rows_within(listHeight, ITEM_H);

	::Vector2 mouse = GetMousePosition();
	bool isMouseInList = mouse.x >= listX && mouse.x < listX + listWidth &&
		mouse.y >= listY && mouse.y < listY + listHeight;

	if (isMouseInList)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			listCursor.scroll_by(-static_cast<int>(wheel), visible);
		}
	}

	BeginScissorMode(listX, listY, listWidth, listHeight);

	const size_t scroll = listCursor.first_visible(visible);
	for (size_t entryIndex = scroll; entryIndex < entries.size(); ++entryIndex)
	{
		int itemY = listY + static_cast<int>(entryIndex - scroll) * ITEM_H;
		if (itemY + ITEM_H > listY + listHeight)
		{
			break;
		}

		bool isSelected = (entryIndex == listCursor.current_row());
		bool hovered = isMouseInList && mouse.y >= itemY && mouse.y < itemY + ITEM_H;

		if (isSelected)
		{
			DrawRectangle(listX, itemY, listWidth, ITEM_H, Color{ 60, 60, 0, 220 });
		}
		else if (hovered)
		{
			DrawRectangle(listX, itemY, listWidth, ITEM_H, Color{ 30, 30, 30, 180 });
		}

		TileRef tile = (tab == 0)
			? (contentRegistry ? contentRegistry->get_tile(entries[entryIndex].itemKey) : TileRef{})
			: monsters.get_tile(static_cast<MonsterId>(entries[entryIndex].entityKey));

		renderer.draw_tile_screen_sized(Vector2D{ listX + PAD, itemY + (ITEM_H - TILE_SZ) / 2 }, tile, TILE_SZ);

		constexpr Color SELECTED_TEXT{ 255, 255, 100, 255 };
		constexpr Color PLAIN_TEXT{ 200, 200, 200, 255 };
		Color textColor = isSelected ? SELECTED_TEXT : PLAIN_TEXT;
		renderer.draw_text_color(Vector2D{ listX + PAD + TILE_SZ + 4, itemY + (ITEM_H - 16) / 2 }, entries[entryIndex].name, textColor);

		if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			listCursor.select(entryIndex);
		}
	}

	EndScissorMode();
}

void ContentEditor::draw_browser(
	const Renderer& renderer,
	MonsterRegistry& monsters,
	int panelX,
	int panelY,
	int browserWidth,
	int browserHeight)
{
	constexpr int BROWSER_TILE = 36;
	constexpr int PAD = 10;
	constexpr int SUB_HEADER_H = 28;

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
	};

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(browserSheet)))
	{
		advance_sheet(1);
	}

	if (IsKeyPressed(KEY_LEFT))
	{
		advance_sheet(-1);
	}
	if (IsKeyPressed(KEY_RIGHT))
	{
		advance_sheet(1);
	}

	const int sheetColumns = renderer.get_sheet_cols(static_cast<TileSheet>(browserSheet));
	const int sheetRows = renderer.get_sheet_rows(static_cast<TileSheet>(browserSheet));

	DrawRectangle(panelX, panelY, browserWidth, SUB_HEADER_H, Color{ 15, 15, 30, 255 });

	std::string header = std::format(
		"Sheet: {} ({}/{})  --  {}x{} tiles",
		renderer.get_sheet_name(static_cast<TileSheet>(browserSheet)),
		browserSheet + 1,
		totalSheets,
		sheetColumns,
		sheetRows);

	renderer.draw_text_color(Vector2D{ panelX + PAD, panelY + 4 }, header, Color{ 200, 200, 120, 255 });

	const int gridY = panelY + SUB_HEADER_H;
	const int gridHeight = browserHeight - SUB_HEADER_H;

	::Vector2 mouse = GetMousePosition();
	bool isMouseInBrowser = mouse.x >= panelX && mouse.x < panelX + browserWidth &&
		mouse.y >= gridY && mouse.y < gridY + gridHeight;

	if (isMouseInBrowser)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			browserScroll -= static_cast<int>(wheel);
			browserScroll = std::clamp(browserScroll, 0, std::max(0, sheetRows - 1));
		}
	}

	TileRef selectedTile = current_tile(monsters);

	BeginScissorMode(panelX, gridY, browserWidth, gridHeight);

	for (int row = browserScroll; row < sheetRows; ++row)
	{
		int cellPixelY = gridY + (row - browserScroll) * (BROWSER_TILE + 2);
		if (cellPixelY >= gridY + gridHeight)
		{
			break;
		}

		for (int col = 0; col < sheetColumns; ++col)
		{
			int cellPixelX = panelX + PAD + col * (BROWSER_TILE + 2);
			TileRef tile{ static_cast<TileSheet>(browserSheet), col, row };

			bool isSelected = (tile == selectedTile);
			bool isTileHovered = mouse.x >= cellPixelX && mouse.x < cellPixelX + BROWSER_TILE &&
				mouse.y >= cellPixelY && mouse.y < cellPixelY + BROWSER_TILE;

			if (isSelected)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			}
			else if (isTileHovered)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 40, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tile, BROWSER_TILE);

			if (isSelected)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
			}

			if (isTileHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isMouseInBrowser)
			{
				assign_tile(tile, monsters);
			}
		}
	}

	EndScissorMode();
}

void ContentEditor::draw_hint_bar(const Renderer& renderer) const
{
	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();
	constexpr int HINT_H = 28;

	int hintY = screenHeight - HINT_H;
	DrawRectangle(0, hintY, screenWidth, HINT_H, Color{ 20, 20, 40, 255 });

	bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;
	std::string hint = std::format(
		"Tab:switch tab   Up/Down:select   Left/Right:sheet   Click tile:assign   Ctrl+S:save{}",
		isSaveRecent ? "   -- SAVED!" : "");

	constexpr Color SAVED_HINT{ 100, 255, 100, 255 };
	constexpr Color PLAIN_HINT{ 160, 160, 120, 255 };
	Color hintColor = isSaveRecent ? SAVED_HINT : PLAIN_HINT;
	renderer.draw_text_color(Vector2D{ 8, hintY + 4 }, hint, hintColor);
}

void ContentEditor::handle_keyboard(size_t visibleRows)
{
	if (IsKeyPressed(KEY_TAB))
	{
		tab = 1 - tab;
		listCursor.reset(active_entries().size());
	}

	// An empty tab has no row to move from, so both keys do nothing there.
	if (IsKeyPressed(KEY_UP) && listCursor.can_move_up())
	{
		listCursor.move_up();
	}

	// A screen too short for one row has nowhere to scroll the cursor to.
	if (IsKeyPressed(KEY_DOWN) && listCursor.can_move_down() && visibleRows > 0)
	{
		listCursor.move_down(visibleRows);
	}

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	if (ctrl && IsKeyPressed(KEY_S))
	{
		if (contentRegistry)
		{
			ContentRegistryIO::save(*contentRegistry, Paths::CONTENT_TILES);
		}
		lastSaveTime = GetTime();
	}
}
