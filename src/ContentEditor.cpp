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
		listCursor = 0;
		listScroll = 0;
	}
}

TileRef ContentEditor::current_tile(const MonsterRegistry& monsters) const
{
	const auto& entries = active_entries();
	if (entries.empty())
	{
		return TileRef{};
	}

	const auto& sel = entries[listCursor];
	if (tab == 0)
	{
		return contentRegistry ? contentRegistry->get_tile(sel.itemKey) : TileRef{};
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

	const auto& sel = entries[listCursor];
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

	handle_keyboard();

	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();

	constexpr int HEADER_H = 56;
	constexpr int HINT_H = 28;
	constexpr int LIST_W = 420;

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 220 });

	draw_header(renderer);

	const int body_y = HEADER_H;
	const int body_h = screenHeight - HEADER_H - HINT_H;

	draw_list(renderer, monsters, 0, body_y, LIST_W, body_h);
	draw_browser(renderer, monsters, LIST_W, body_y, screenWidth - LIST_W, body_h);
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

		bool click_tab = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			mouse.x >= tabPixelX && mouse.x < tabPixelX + TAB_W &&
			mouse.y >= TAB_Y && mouse.y < TAB_Y + TAB_H;

		if (click_tab)
		{
			tab = tabIndex;
			listCursor = 0;
			listScroll = 0;
		}
	}
}

void ContentEditor::draw_list(
	const Renderer& renderer,
	const MonsterRegistry& monsters,
	int list_x,
	int list_y,
	int list_w,
	int list_h)
{
	constexpr int ITEM_H = 36;
	constexpr int TILE_SZ = 28;
	constexpr int PAD = 8;

	DrawRectangle(list_x, list_y, list_w, list_h, Color{ 10, 10, 20, 255 });
	DrawLine(list_x + list_w - 1, list_y, list_x + list_w - 1, list_y + list_h, Color{ 80, 80, 120, 255 });

	const auto& entries = active_entries();
	int total = static_cast<int>(entries.size());
	int visible = list_h / ITEM_H;
	int max_scroll = std::max(0, total - visible);
	listScroll = std::clamp(listScroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();
	bool in_list = mouse.x >= list_x && mouse.x < list_x + list_w &&
		mouse.y >= list_y && mouse.y < list_y + list_h;

	if (in_list)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			listScroll = std::clamp(listScroll - static_cast<int>(wheel), 0, max_scroll);
		}
	}

	BeginScissorMode(list_x, list_y, list_w, list_h);

	for (int entryIndex = listScroll; entryIndex < total; ++entryIndex)
	{
		int itemY = list_y + (entryIndex - listScroll) * ITEM_H;
		if (itemY + ITEM_H > list_y + list_h)
		{
			break;
		}

		bool is_sel = (entryIndex == listCursor);
		bool hovered = in_list && mouse.y >= itemY && mouse.y < itemY + ITEM_H;

		if (is_sel)
		{
			DrawRectangle(list_x, itemY, list_w, ITEM_H, Color{ 60, 60, 0, 220 });
		}
		else if (hovered)
		{
			DrawRectangle(list_x, itemY, list_w, ITEM_H, Color{ 30, 30, 30, 180 });
		}

		TileRef tile = (tab == 0)
			? (contentRegistry ? contentRegistry->get_tile(entries[entryIndex].itemKey) : TileRef{})
			: monsters.get_tile(static_cast<MonsterId>(entries[entryIndex].entityKey));

		renderer.draw_tile_screen_sized(Vector2D{ list_x + PAD, itemY + (ITEM_H - TILE_SZ) / 2 }, tile, TILE_SZ);

		Color textColor = is_sel ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
		renderer.draw_text_color(Vector2D{ list_x + PAD + TILE_SZ + 4, itemY + (ITEM_H - 16) / 2 }, entries[entryIndex].name, textColor);

		if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			listCursor = entryIndex;
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

	const int total_sheets = renderer.get_loaded_sheet_count();
	auto advance_sheet = [&](int dir)
	{
		for (int sheetsTried = 0; sheetsTried < total_sheets; ++sheetsTried)
		{
			browserSheet = (browserSheet + dir + total_sheets) % total_sheets;
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

	const int sheet_cols = renderer.get_sheet_cols(static_cast<TileSheet>(browserSheet));
	const int sheet_rows = renderer.get_sheet_rows(static_cast<TileSheet>(browserSheet));

	DrawRectangle(panelX, panelY, browserWidth, SUB_HEADER_H, Color{ 15, 15, 30, 255 });

	std::string hdr = std::format(
		"Sheet: {} ({}/{})  --  {}x{} tiles",
		renderer.get_sheet_name(static_cast<TileSheet>(browserSheet)),
		browserSheet + 1,
		total_sheets,
		sheet_cols,
		sheet_rows);

	renderer.draw_text_color(Vector2D{ panelX + PAD, panelY + 4 }, hdr, Color{ 200, 200, 120, 255 });

	const int grid_y = panelY + SUB_HEADER_H;
	const int grid_h = browserHeight - SUB_HEADER_H;

	::Vector2 mouse = GetMousePosition();
	bool in_browser = mouse.x >= panelX && mouse.x < panelX + browserWidth &&
		mouse.y >= grid_y && mouse.y < grid_y + grid_h;

	if (in_browser)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			browserScroll -= static_cast<int>(wheel);
			browserScroll = std::clamp(browserScroll, 0, std::max(0, sheet_rows - 1));
		}
	}

	TileRef selected_tile = current_tile(monsters);

	BeginScissorMode(panelX, grid_y, browserWidth, grid_h);

	for (int row = browserScroll; row < sheet_rows; ++row)
	{
		int cellPixelY = grid_y + (row - browserScroll) * (BROWSER_TILE + 2);
		if (cellPixelY >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int cellPixelX = panelX + PAD + col * (BROWSER_TILE + 2);
			TileRef tid{ static_cast<TileSheet>(browserSheet), col, row };

			bool is_sel = (tid == selected_tile);
			bool hovered_tile = mouse.x >= cellPixelX && mouse.x < cellPixelX + BROWSER_TILE &&
				mouse.y >= cellPixelY && mouse.y < cellPixelY + BROWSER_TILE;

			if (is_sel)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			}
			else if (hovered_tile)
			{
				DrawRectangle(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 40, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tid, BROWSER_TILE);

			if (is_sel)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
			}

			if (hovered_tile && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_browser)
			{
				assign_tile(tid, monsters);
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

	int hint_y = screenHeight - HINT_H;
	DrawRectangle(0, hint_y, screenWidth, HINT_H, Color{ 20, 20, 40, 255 });

	bool saved_flash = (GetTime() - lastSaveTime) < 2.0;
	std::string hint = std::format(
		"Tab:switch tab   Up/Down:select   Left/Right:sheet   Click tile:assign   Ctrl+S:save{}",
		saved_flash ? "   -- SAVED!" : "");

	Color hintColor = saved_flash ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	renderer.draw_text_color(Vector2D{ 8, hint_y + 4 }, hint, hintColor);
}

void ContentEditor::handle_keyboard()
{
	if (IsKeyPressed(KEY_TAB))
	{
		tab = 1 - tab;
		listCursor = 0;
		listScroll = 0;
	}

	if (IsKeyPressed(KEY_UP))
	{
		listCursor = std::max(0, listCursor - 1);
		listScroll = std::min(listScroll, listCursor);
	}

	if (IsKeyPressed(KEY_DOWN))
	{
		int max_idx = static_cast<int>(active_entries().size()) - 1;
		listCursor = std::min(max_idx, listCursor + 1);
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
