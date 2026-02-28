// file: ContentEditor.cpp
#include <algorithm>
#include <format>
#include <raylib.h>
#include <string>

#include "../Core/Paths.h"
#include "../Factories/MonsterCreator.h"
#include "../Items/ItemClassification.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/TileId.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/ContentRegistryIO.h"
#include "ContentEditor.h"

// ---------------------------------------------------------------------------
// Static entity tables
// ---------------------------------------------------------------------------

namespace
{

struct ItemEntry
{
	ItemId id;
	const char* name;
};

struct MonsterEntry
{
	MonsterId id;
	const char* name;
};

constexpr ItemEntry ITEM_TABLE[] = {
	{ ItemId::HEALTH_POTION, "Health Potion" },
	{ ItemId::POTION_OF_EXTRA_HEALING, "Potion of Extra Healing" },
	{ ItemId::MANA_POTION, "Mana Potion" },
	{ ItemId::INVISIBILITY_POTION, "Invisibility Potion" },
	{ ItemId::POTION_OF_GIANT_STRENGTH, "Potion of Giant Strength" },
	{ ItemId::POTION_OF_FIRE_RESISTANCE, "Potion of Fire Resistance" },
	{ ItemId::POTION_OF_COLD_RESISTANCE, "Potion of Cold Resistance" },
	{ ItemId::POTION_OF_SPEED, "Potion of Speed" },
	{ ItemId::SCROLL_LIGHTNING, "Scroll of Lightning" },
	{ ItemId::SCROLL_FIREBALL, "Scroll of Fireball" },
	{ ItemId::SCROLL_CONFUSION, "Scroll of Confusion" },
	{ ItemId::SCROLL_TELEPORT, "Scroll of Teleport" },
	{ ItemId::DAGGER, "Dagger" },
	{ ItemId::SHORT_SWORD, "Short Sword" },
	{ ItemId::LONG_SWORD, "Long Sword" },
	{ ItemId::BASTARD_SWORD, "Bastard Sword" },
	{ ItemId::TWO_HANDED_SWORD, "Two-Handed Sword" },
	{ ItemId::GREAT_SWORD, "Great Sword" },
	{ ItemId::SCIMITAR, "Scimitar" },
	{ ItemId::RAPIER, "Rapier" },
	{ ItemId::HAND_AXE, "Hand Axe" },
	{ ItemId::BATTLE_AXE, "Battle Axe" },
	{ ItemId::GREAT_AXE, "Great Axe" },
	{ ItemId::WAR_HAMMER, "War Hammer" },
	{ ItemId::MACE, "Mace" },
	{ ItemId::MORNING_STAR, "Morning Star" },
	{ ItemId::FLAIL, "Flail" },
	{ ItemId::CLUB, "Club" },
	{ ItemId::QUARTERSTAFF, "Quarterstaff" },
	{ ItemId::STAFF, "Staff" },
	{ ItemId::SHORT_BOW, "Short Bow" },
	{ ItemId::LONG_BOW, "Long Bow" },
	{ ItemId::COMPOSITE_BOW, "Composite Bow" },
	{ ItemId::LIGHT_CROSSBOW, "Light Crossbow" },
	{ ItemId::HEAVY_CROSSBOW, "Heavy Crossbow" },
	{ ItemId::CROSSBOW, "Crossbow" },
	{ ItemId::SLING, "Sling" },
	{ ItemId::PADDED_ARMOR, "Padded Armor" },
	{ ItemId::LEATHER_ARMOR, "Leather Armor" },
	{ ItemId::STUDDED_LEATHER, "Studded Leather" },
	{ ItemId::HIDE_ARMOR, "Hide Armor" },
	{ ItemId::RING_MAIL, "Ring Mail" },
	{ ItemId::SCALE_MAIL, "Scale Mail" },
	{ ItemId::CHAIN_MAIL, "Chain Mail" },
	{ ItemId::BRIGANDINE, "Brigandine" },
	{ ItemId::SPLINT_MAIL, "Splint Mail" },
	{ ItemId::BANDED_MAIL, "Banded Mail" },
	{ ItemId::PLATE_MAIL, "Plate Mail" },
	{ ItemId::FIELD_PLATE, "Field Plate" },
	{ ItemId::FULL_PLATE, "Full Plate" },
	{ ItemId::SMALL_SHIELD, "Small Shield" },
	{ ItemId::MEDIUM_SHIELD, "Medium Shield" },
	{ ItemId::LARGE_SHIELD, "Large Shield" },
	{ ItemId::HELM_OF_BRILLIANCE, "Helm of Brilliance" },
	{ ItemId::HELM_OF_TELEPORTATION, "Helm of Teleportation" },
	{ ItemId::HELM_OF_TELEPATHY, "Helm of Telepathy" },
	{ ItemId::HELM_OF_UNDERWATER_ACTION, "Helm of Underwater Action" },
	{ ItemId::RING_OF_PROTECTION_PLUS_1, "Ring of Protection +1" },
	{ ItemId::RING_OF_PROTECTION_PLUS_2, "Ring of Protection +2" },
	{ ItemId::RING_OF_FREE_ACTION, "Ring of Free Action" },
	{ ItemId::RING_OF_REGENERATION, "Ring of Regeneration" },
	{ ItemId::RING_OF_INVISIBILITY, "Ring of Invisibility" },
	{ ItemId::RING_OF_FIRE_RESISTANCE, "Ring of Fire Resistance" },
	{ ItemId::RING_OF_COLD_RESISTANCE, "Ring of Cold Resistance" },
	{ ItemId::RING_OF_SPELL_STORING, "Ring of Spell Storing" },
	{ ItemId::AMULET_OF_HEALTH, "Amulet of Health" },
	{ ItemId::AMULET_OF_WISDOM, "Amulet of Wisdom" },
	{ ItemId::AMULET_OF_PROTECTION, "Amulet of Protection" },
	{ ItemId::AMULET_OF_OGRE_POWER, "Amulet of Ogre Power" },
	{ ItemId::AMULET_OF_YENDOR, "Amulet of Yendor" },
	{ ItemId::GAUNTLETS_OF_OGRE_POWER, "Gauntlets of Ogre Power" },
	{ ItemId::GAUNTLETS_OF_DEXTERITY, "Gauntlets of Dexterity" },
	{ ItemId::GAUNTLETS_OF_SWIMMING_AND_CLIMBING, "Gauntlets of Swimming & Climbing" },
	{ ItemId::GAUNTLETS_OF_FUMBLING, "Gauntlets of Fumbling" },
	{ ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH, "Girdle of Hill Giant Strength" },
	{ ItemId::GIRDLE_OF_STONE_GIANT_STRENGTH, "Girdle of Stone Giant Strength" },
	{ ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH, "Girdle of Frost Giant Strength" },
	{ ItemId::GIRDLE_OF_FIRE_GIANT_STRENGTH, "Girdle of Fire Giant Strength" },
	{ ItemId::GIRDLE_OF_CLOUD_GIANT_STRENGTH, "Girdle of Cloud Giant Strength" },
	{ ItemId::GIRDLE_OF_STORM_GIANT_STRENGTH, "Girdle of Storm Giant Strength" },
	{ ItemId::BOOTS_OF_SPEED, "Boots of Speed" },
	{ ItemId::BOOTS_OF_ELVENKIND, "Boots of Elvenkind" },
	{ ItemId::CLOAK_OF_PROTECTION, "Cloak of Protection" },
	{ ItemId::CLOAK_OF_DISPLACEMENT, "Cloak of Displacement" },
	{ ItemId::CLOAK_OF_ELVENKIND, "Cloak of Elvenkind" },
	{ ItemId::FOOD_RATION, "Food Ration" },
	{ ItemId::BREAD, "Bread" },
	{ ItemId::MEAT, "Meat" },
	{ ItemId::FRUIT, "Fruit" },
	{ ItemId::GOLD_COIN, "Gold" },
	{ ItemId::GEM, "Gem" },
	{ ItemId::TORCH, "Torch" },
	{ ItemId::ROPE, "Rope" },
	{ ItemId::LOCKPICK, "Lockpick" },
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

void ContentEditor::toggle()
{
	if (m_active)
	{
		// Closing: persist all assignments immediately.
		ContentRegistryIO::save(ContentRegistry::instance(), Paths::CONTENT_TILES);
	}

	m_active = !m_active;

	if (m_active && m_item_entries.empty())
	{
		for (const auto& row : ITEM_TABLE)
		{
			m_item_entries.push_back({ row.name, static_cast<int>(row.id) });
		}
		for (const auto& row : MONSTER_TABLE)
		{
			m_monster_entries.push_back({ row.name, static_cast<int>(row.id) });
		}
		m_list_cursor = 0;
		m_list_scroll = 0;
	}
}

int ContentEditor::current_tile() const
{
	const auto& entries = active_entries();
	if (entries.empty())
		return 0;
	int key = entries[m_list_cursor].entity_key;
	if (m_tab == 0)
		return ContentRegistry::instance().get_tile(static_cast<ItemId>(key));
	return ContentRegistry::instance().get_tile(static_cast<MonsterId>(key));
}

void ContentEditor::assign_tile(int tile_id)
{
	auto& entries = active_entries();
	if (entries.empty())
		return;
	int key = entries[m_list_cursor].entity_key;
	if (m_tab == 0)
		ContentRegistry::instance().set_tile(static_cast<ItemId>(key), tile_id);
	else
		ContentRegistry::instance().set_tile(static_cast<MonsterId>(key), tile_id);
}

// ---------------------------------------------------------------------------
// update_and_render
// ---------------------------------------------------------------------------

void ContentEditor::update_and_render(const Renderer& renderer)
{
	if (!m_active)
		return;

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
			m_list_scroll = std::clamp(m_list_scroll - static_cast<int>(wheel), 0, max_scroll);
	}

	BeginScissorMode(list_x, list_y, list_w, list_h);

	for (int i = m_list_scroll; i < total; ++i)
	{
		int iy = list_y + (i - m_list_scroll) * ITEM_H;
		if (iy + ITEM_H > list_y + list_h)
			break;

		bool is_sel = (i == m_list_cursor);
		bool hovered = in_list && mouse.y >= iy && mouse.y < iy + ITEM_H;

		if (is_sel)
			DrawRectangle(list_x, iy, list_w, ITEM_H, Color{ 60, 60, 0, 220 });
		else if (hovered)
			DrawRectangle(list_x, iy, list_w, ITEM_H, Color{ 30, 30, 30, 180 });

		int tile_id = (m_tab == 0)
			? ContentRegistry::instance().get_tile(static_cast<ItemId>(entries[i].entity_key))
			: ContentRegistry::instance().get_tile(static_cast<MonsterId>(entries[i].entity_key));

		renderer.draw_tile_screen_sized(list_x + PAD, iy + (ITEM_H - TILE_SZ) / 2, tile_id, TILE_SZ);

		Color tc = is_sel ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
		renderer.draw_text_color(list_x + PAD + TILE_SZ + 4, iy + (ITEM_H - 16) / 2, entries[i].name, tc);

		if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			m_list_cursor = i;
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
				break;
		}
		m_browser_scroll = 0;
	};

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(m_browser_sheet)))
		advance_sheet(1);

	if (IsKeyPressed(KEY_LEFT))
		advance_sheet(-1);
	if (IsKeyPressed(KEY_RIGHT))
		advance_sheet(1);

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

	int selected_tile = current_tile();

	BeginScissorMode(ox, grid_y, bw, grid_h);

	for (int row = m_browser_scroll; row < sheet_rows; ++row)
	{
		int py = grid_y + (row - m_browser_scroll) * (BROWSER_TILE + 2);
		if (py >= grid_y + grid_h)
			break;

		for (int col = 0; col < sheet_cols; ++col)
		{
			int px = ox + PAD + col * (BROWSER_TILE + 2);
			int tid = make_tile(static_cast<TileSheet>(m_browser_sheet), col, row);

			bool is_sel = (tid == selected_tile);
			bool hovered_tile = mouse.x >= px && mouse.x < px + BROWSER_TILE &&
				mouse.y >= py && mouse.y < py + BROWSER_TILE;

			if (is_sel)
				DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
			else if (hovered_tile)
				DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 40, 40, 20, 160 });

			renderer.draw_tile_screen_sized(px, py, tid, BROWSER_TILE);

			if (is_sel)
				DrawRectangleLines(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });

			if (hovered_tile && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_browser)
				assign_tile(tid);
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
		ContentRegistryIO::save(ContentRegistry::instance(), Paths::CONTENT_TILES);
		m_last_save_time = GetTime();
	}
}
