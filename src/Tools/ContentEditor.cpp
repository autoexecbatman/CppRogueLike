// file: ContentEditor.cpp
#include <algorithm>
#include <format>
#include <string>

#include <raylib.h>

#include "../Core/Paths.h"
#include "../Factories/ItemCreator.h"
#include "../Factories/MonsterCreator.h"
#include "../Renderer/Renderer.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/ContentRegistryIO.h"
#include "ContentEditor.h"

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

void ContentEditor::toggle(ContentRegistry& registry)
{
	m_registry = &registry;

	if (m_active)
	{
		// Closing: persist all assignments immediately.
		ContentRegistryIO::save(*m_registry, Paths::CONTENT_TILES);
		MonsterCreator::save(Paths::MONSTERS);
	}

	m_active = !m_active;

	if (m_active && m_item_entries.empty())
	{
		for (const auto& key : ItemCreator::get_all_keys())
		{
			const auto& p = ItemCreator::get_params(key);
			m_item_entries.push_back({ std::string(p.name), std::string(key), 0 });
		}
		for (const auto& row : MONSTER_TABLE)
		{
			m_monster_entries.push_back({ row.name, {}, static_cast<int>(row.id) });
		}
		m_list_cursor = 0;
		m_list_scroll = 0;
	}
}

TileRef ContentEditor::current_tile() const
{
	const auto& entries = active_entries();
	if (entries.empty())
	{
		return TileRef{};
	}

	const auto& sel = entries[m_list_cursor];
	if (m_tab == 0)
	{
		return m_registry ? m_registry->get_tile(sel.item_key) : TileRef{};
	}

	return MonsterCreator::get_tile(static_cast<MonsterId>(sel.entity_key));
}

void ContentEditor::assign_tile(TileRef tile)
{
	auto& entries = active_entries();
	if (entries.empty())
	{
		return;
	}

	const auto& sel = entries[m_list_cursor];
	if (m_tab == 0)
	{
		if (m_registry)
		{
			m_registry->set_tile(sel.item_key, tile);
		}
	}
	else
	{
		MonsterCreator::set_tile(static_cast<MonsterId>(sel.entity_key), tile);
	}
}

// ---------------------------------------------------------------------------
// update_and_render
// ---------------------------------------------------------------------------

void ContentEditor::update_and_render(const Renderer& renderer, ContentRegistry& registry)
{
	if (!m_active)
	{
		return;
	}
	m_registry = &registry;

	handle_keyboard();

	const int sw = renderer.get_screen_width();
	const int sh = renderer.get_screen_height();

	constexpr int HEADER_H = 56;
	constexpr int HINT_H = 28;
	constexpr int LIST_W = 420;

	DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 220 });

	draw_header(renderer);

	const int body_y = HEADER_H;
	const int body_h = sh - HEADER_H - HINT_H;

	draw_list(renderer, 0, body_y, LIST_W, body_h);
	draw_browser(renderer, LIST_W, body_y, sw - LIST_W, body_h);
	draw_hint_bar(renderer);
}

void ContentEditor::draw_header(const Renderer& renderer)
{
	const int sw = renderer.get_screen_width();
	constexpr int HEADER_H = 56;
	constexpr int TAB_W = 110;
	constexpr int TAB_H = 28;
	constexpr int TAB_Y = 20;

	DrawRectangle(0, 0, sw, HEADER_H, Color{ 20, 20, 40, 255 });
	renderer.draw_text_color(8, 4, "CONTENT EDITOR", Color{ 255, 255, 180, 255 });

	const char* TAB_NAMES[2] = { "Items", "Monsters" };
	::Vector2 mouse = GetMousePosition();

	for (int t = 0; t < 2; ++t)
	{
		int tx = 8 + t * (TAB_W + 4);
		bool sel = (m_tab == t);
		Color bg = sel ? Color{ 80, 80, 0, 255 } : Color{ 30, 30, 60, 255 };
		Color tc = sel ? Color{ 255, 255, 100, 255 } : Color{ 160, 160, 160, 255 };

		DrawRectangle(tx, TAB_Y, TAB_W, TAB_H, bg);
		DrawRectangleLines(tx, TAB_Y, TAB_W, TAB_H, Color{ 100, 100, 60, 255 });
		renderer.draw_text_color(tx + 8, TAB_Y + 4, TAB_NAMES[t], tc);

		bool click_tab = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			mouse.x >= tx && mouse.x < tx + TAB_W &&
			mouse.y >= TAB_Y && mouse.y < TAB_Y + TAB_H;

		if (click_tab)
		{
			m_tab = t;
			m_list_cursor = 0;
			m_list_scroll = 0;
		}
	}
}

void ContentEditor::draw_list(
	const Renderer& renderer,
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
	m_list_scroll = std::clamp(m_list_scroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();
	bool in_list = mouse.x >= list_x && mouse.x < list_x + list_w &&
		mouse.y >= list_y && mouse.y < list_y + list_h;

	if (in_list)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			m_list_scroll = std::clamp(m_list_scroll - static_cast<int>(wheel), 0, max_scroll);
		}
	}

	BeginScissorMode(list_x, list_y, list_w, list_h);

	for (int i = m_list_scroll; i < total; ++i)
	{
		int iy = list_y + (i - m_list_scroll) * ITEM_H;
		if (iy + ITEM_H > list_y + list_h)
		{
			break;
		}

		bool is_sel = (i == m_list_cursor);
		bool hovered = in_list && mouse.y >= iy && mouse.y < iy + ITEM_H;

		if (is_sel)
		{
			DrawRectangle(list_x, iy, list_w, ITEM_H, Color{ 60, 60, 0, 220 });
		}
		else if (hovered)
		{
			DrawRectangle(list_x, iy, list_w, ITEM_H, Color{ 30, 30, 30, 180 });
		}

		TileRef tile = (m_tab == 0)
			? (m_registry ? m_registry->get_tile(entries[i].item_key) : TileRef{})
			: MonsterCreator::get_tile(static_cast<MonsterId>(entries[i].entity_key));

		renderer.draw_tile_screen_sized(list_x + PAD, iy + (ITEM_H - TILE_SZ) / 2, tile, TILE_SZ);

		Color tc = is_sel ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
		renderer.draw_text_color(list_x + PAD + TILE_SZ + 4, iy + (ITEM_H - 16) / 2, entries[i].name, tc);

		if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			m_list_cursor = i;
		}
	}

	EndScissorMode();
}

void ContentEditor::draw_browser(
	const Renderer& renderer,
	int ox,
	int oy,
	int bw,
	int bh)
{
	constexpr int BROWSER_TILE = 36;
	constexpr int PAD = 10;
	constexpr int SUB_HEADER_H = 28;

	const int total_sheets = renderer.get_loaded_sheet_count();
	auto advance_sheet = [&](int dir)
	{
		for (int i = 0; i < total_sheets; ++i)
		{
			m_browser_sheet = (m_browser_sheet + dir + total_sheets) % total_sheets;
			if (renderer.sheet_is_loaded(static_cast<TileSheet>(m_browser_sheet)))
			{
				break;
			}
		}
		m_browser_scroll = 0;
	};

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(m_browser_sheet)))
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

	const int sheet_cols = renderer.get_sheet_cols(static_cast<TileSheet>(m_browser_sheet));
	const int sheet_rows = renderer.get_sheet_rows(static_cast<TileSheet>(m_browser_sheet));

	DrawRectangle(ox, oy, bw, SUB_HEADER_H, Color{ 15, 15, 30, 255 });

	std::string hdr = std::format(
		"Sheet: {} ({}/{})  --  {}x{} tiles",
		renderer.get_sheet_name(static_cast<TileSheet>(m_browser_sheet)),
		m_browser_sheet + 1,
		total_sheets,
		sheet_cols,
		sheet_rows);

	renderer.draw_text_color(ox + PAD, oy + 4, hdr, Color{ 200, 200, 120, 255 });

	const int grid_y = oy + SUB_HEADER_H;
	const int grid_h = bh - SUB_HEADER_H;

	::Vector2 mouse = GetMousePosition();
	bool in_browser = mouse.x >= ox && mouse.x < ox + bw &&
		mouse.y >= grid_y && mouse.y < grid_y + grid_h;

	if (in_browser)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			m_browser_scroll -= static_cast<int>(wheel);
			m_browser_scroll = std::clamp(m_browser_scroll, 0, std::max(0, sheet_rows - 1));
		}
	}

	TileRef selected_tile = current_tile();

	BeginScissorMode(ox, grid_y, bw, grid_h);

	for (int row = m_browser_scroll; row < sheet_rows; ++row)
	{
		int py = grid_y + (row - m_browser_scroll) * (BROWSER_TILE + 2);
		if (py >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int px = ox + PAD + col * (BROWSER_TILE + 2);
			TileRef tid{ static_cast<TileSheet>(m_browser_sheet), col, row };

			bool is_sel = (tid == selected_tile);
			bool hovered_tile = mouse.x >= px && mouse.x < px + BROWSER_TILE &&
				mouse.y >= py && mouse.y < py + BROWSER_TILE;

			if (is_sel)
			{
				DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			}
			else if (hovered_tile)
			{
				DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 40, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(px, py, tid, BROWSER_TILE);

			if (is_sel)
			{
				DrawRectangleLines(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
			}

			if (hovered_tile && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_browser)
			{
				assign_tile(tid);
			}
		}
	}

	EndScissorMode();
}

void ContentEditor::draw_hint_bar(const Renderer& renderer) const
{
	const int sw = renderer.get_screen_width();
	const int sh = renderer.get_screen_height();
	constexpr int HINT_H = 28;

	int hint_y = sh - HINT_H;
	DrawRectangle(0, hint_y, sw, HINT_H, Color{ 20, 20, 40, 255 });

	bool saved_flash = (GetTime() - m_last_save_time) < 2.0;
	std::string hint = std::format(
		"Tab:switch tab   Up/Down:select   Left/Right:sheet   Click tile:assign   Ctrl+S:save{}",
		saved_flash ? "   -- SAVED!" : "");

	Color hc = saved_flash ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	renderer.draw_text_color(8, hint_y + 4, hint, hc);
}

void ContentEditor::handle_keyboard()
{
	if (IsKeyPressed(KEY_TAB))
	{
		m_tab = 1 - m_tab;
		m_list_cursor = 0;
		m_list_scroll = 0;
	}

	if (IsKeyPressed(KEY_UP))
	{
		m_list_cursor = std::max(0, m_list_cursor - 1);
		m_list_scroll = std::min(m_list_scroll, m_list_cursor);
	}

	if (IsKeyPressed(KEY_DOWN))
	{
		int max_idx = static_cast<int>(active_entries().size()) - 1;
		m_list_cursor = std::min(max_idx, m_list_cursor + 1);
	}

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	if (ctrl && IsKeyPressed(KEY_S))
	{
		if (m_registry)
		{
			ContentRegistryIO::save(*m_registry, Paths::CONTENT_TILES);
		}
		m_last_save_time = GetTime();
	}
}
