#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <memory>
#include <raylib.h>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Actor.h"
#include "EquipmentSlot.h"
#include "InventoryOperations.h"
#include "Pickable.h"
#include "Player.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "WeaponDamageRegistry.h"
#include "WeightTier.h"
#include "GameContext.h"
#include "ItemClassification.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "MessageSystem.h"
#include "CloseButtonArea.h"
#include "InventoryUI.h"


// Where one text row sits, and where a glyph sits inside it. Drawing and mouse
// hit-testing both go through these, so a click always lands on the row the
// player can see.
static int row_top_y(int tileSize, int row)
{
	return panel_text_row_y(0, tileSize, row);
}

static int row_text_y(int tileSize, int fontSize, int row)
{
	return row_top_y(tileSize, row) + (UI_TEXT_ROW_PITCH - fontSize) / 2;
}

// The row a screen y falls in. Negative above the first row, which the callers
// test for rather than clamping.
static int row_at_y(int tileSize, int screenY)
{
	return (screenY - tileSize) / UI_TEXT_ROW_PITCH;
}

// First row of the detail bar, measured back from the bottom of the screen. The
// content above it gets whatever is left, which is what the list scrolls within.
static int detail_bar_top_row(const Renderer& renderer)
{
	const int rowsThatFit = panel_text_rows_that_fit(
		renderer.get_screen_height(), renderer.get_tile_size(), renderer.get_font_size());
	return rowsThatFit - DETAIL_BAR_ROWS;
}

static int list_content_rows(const Renderer& renderer)
{
	return detail_bar_top_row(renderer) - FIRST_CONTENT_ROW;
}

int InventoryUI::screen_cols(GameContext& ctx) const
{
	return ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
}

int InventoryUI::screen_rows(GameContext& ctx) const
{
	return ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
}

void InventoryUI::draw_frame(GameContext& ctx)
{
	assert(ctx.renderer && "InventoryUI::draw_frame called without a renderer");

	int tileSize = ctx.renderer->get_tile_size();
	int vcols = screen_cols(ctx);
	int screenW = ctx.renderer->get_screen_width();
	int screenH = ctx.renderer->get_screen_height();
	int fontOff = (tileSize - ctx.renderer->get_font_size()) / 2;

	ctx.renderer->draw_frame_pixels(Vector2D{ 0, 0 }, screenW, screenH, *ctx.tileConfig);

	std::string_view title = "INVENTORY";
	int titleW = ctx.renderer->measure_text(title);
	int titleX = (screenW - titleW) / 2;
	ctx.renderer->draw_text(Vector2D{ titleX, fontOff }, title, YELLOW_BLACK_PAIR);

	CloseButtonArea closeBtn = CloseButtonArea(*ctx.renderer, vcols);
	ctx.renderer->draw_text(Vector2D{ closeBtn.get_x(), closeBtn.get_y() }, "[X]", RED_BLACK_PAIR);

	// Draw weight info with tier on the right side
	int currentWeight = InventoryOperations::get_total_weight(playerRef);
	int maxWeight = InventoryOperations::get_max_weight(playerRef, *ctx.dataManager);
	WeightTier tier = get_weight_tier(currentWeight, maxWeight);

	std::string tierName;
	int tierColor = WHITE_BLACK_PAIR;
	switch (tier)
	{
	case WeightTier::LIGHT:
	{
		tierName = "LIGHT";
		tierColor = WHITE_BLACK_PAIR;
		break;
	}
	case WeightTier::MODERATE:
	{
		tierName = "MODERATE";
		tierColor = YELLOW_BLACK_PAIR;
		break;
	}
	case WeightTier::HEAVY:
	{
		tierName = "HEAVY";
		tierColor = MAGENTA_BLACK_PAIR;
		break;
	}
	case WeightTier::OVERENCUMBERED:
	{
		tierName = "OVERENCUMBERED";
		tierColor = RED_BLACK_PAIR;
		break;
	}
	}

	std::string weightInfo = std::format("Weight: {}/{} [{}]", currentWeight, maxWeight, tierName);
	ctx.renderer->draw_text(Vector2D{ tileSize, fontOff }, weightInfo, tierColor);
}

// Draw a full-width white highlight bar at the given tile row.
void InventoryUI::draw_highlight_row(int row, GameContext& ctx)
{
	assert(ctx.renderer && "InventoryUI::draw_highlight_row called without a renderer");

	int tileSize = ctx.renderer->get_tile_size();
	int barX = tileSize;
	int barW = ctx.renderer->get_screen_width() - 2 * tileSize;

	ColorPair pair = ctx.renderer->get_color_pair(BLACK_WHITE_PAIR);
	DrawRectangle(barX, row_top_y(tileSize, row), barW, UI_TEXT_ROW_PITCH, pair.bg);
}

InventoryUI::InventoryUI(Player& player, InventoryScreen startScreen, GameContext& ctx)
	: activeScreen(startScreen),
	  equipmentCursor(0),
	  listCursor(0),
	  scrollOffset(0),
	  filterMode(false),
	  filterSlot(EquipmentSlot::NONE),
	  playerRef(player)
{
	rebuild_item_list(player, ctx);
}

void InventoryUI::menu(GameContext& ctx)
{
	ctx.inputSystem->poll();
	if (!handle_input(playerRef, ctx))
	{
		menu_set_run_false();
		return;
	}

	rebuild_item_list(playerRef, ctx);

	ctx.renderer->begin_frame();

	draw_frame(ctx);
	render_tab_bar(ctx);

	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		render_equipment_screen(playerRef, ctx);
	}
	else
	{
		render_item_list_screen(ctx);
	}

	render_detail_bar(playerRef, ctx);

	ctx.renderer->end_frame();
}

// ============================================================
// Data Building
// ============================================================

void InventoryUI::rebuild_item_list(const Player& player, GameContext& ctx)
{
	listEntries.clear();

	std::vector<Item*> items;
	for (const auto& item : player.inventoryData.items)
	{
		if (!item)
		{
			continue;
		}

		auto is_equipped = [&item](const EquippedItem& eq)
		{
			return eq.item && eq.item->uniqueId == item->uniqueId;
		};
		if (std::ranges::any_of(player.equippedItems, is_equipped))
		{
			continue;
		}

		if (filterMode && !item_fits_slot(*item, filterSlot))
		{
			continue;
		}

		ItemCategory effectiveCat = get_effective_category(*item);

		if (activeScreen == InventoryScreen::USABLES && !is_usable_category(effectiveCat))
		{
			continue;
		}

		items.push_back(item.get());
	}

	const auto& categories = (activeScreen == InventoryScreen::USABLES)
		? std::vector<ItemCategory>(USABLE_CATEGORIES.begin(), USABLE_CATEGORIES.end())
		: std::vector<ItemCategory>(CATEGORY_ORDER.begin(), CATEGORY_ORDER.end());

	for (ItemCategory cat : categories)
	{
		auto matches_category = [this, cat](Item* it)
		{
			return get_effective_category(*it) == cat;
		};

		std::vector<Item*> catItems;
		for (Item* it : items)
		{
			if (matches_category(it))
			{
				catItems.push_back(it);
			}
		}

		if (catItems.empty())
		{
			continue;
		}

		BackpackEntry header;
		header.kind = BackpackEntry::Kind::CATEGORY_HEADER;
		header.category = cat;
		header.headerText = std::format("-- {} --", get_category_name(cat));
		header.item = nullptr;
		listEntries.push_back(std::move(header));

		for (Item* it : catItems)
		{
			BackpackEntry entry;
			entry.kind = BackpackEntry::Kind::ITEM;
			entry.category = cat;
			entry.headerText.clear();
			entry.item = it;
			listEntries.push_back(std::move(entry));
		}
	}

	if (listEntries.empty())
	{
		listCursor = 0;
	}
	else
	{
		if (listCursor >= static_cast<int>(listEntries.size()))
		{
			listCursor = static_cast<int>(listEntries.size()) - 1;
		}
		if (listCursor < 0)
		{
			listCursor = 0;
		}
		if (listEntries[listCursor].kind == BackpackEntry::Kind::CATEGORY_HEADER)
		{
			auto next = get_next_item_index(listCursor, 1);
			if (next)
			{
				listCursor = *next;
			}
		}
	}

	int contentHeight = list_content_rows(*ctx.renderer);
	if (scrollOffset > listCursor)
	{
		scrollOffset = listCursor;
	}
	if (listCursor >= scrollOffset + contentHeight)
	{
		scrollOffset = listCursor - contentHeight + 1;
	}
	if (scrollOffset < 0)
	{
		scrollOffset = 0;
	}
}

bool InventoryUI::item_fits_slot(const Item& item, EquipmentSlot slot) const
{
	switch (slot)
	{
	case EquipmentSlot::HEAD:
	{
		return item.is_helmet();
	}
	case EquipmentSlot::NECK:
	{
		return item.is_amulet();
	}
	case EquipmentSlot::BODY:
	{
		return item.is_armor();
	}
	case EquipmentSlot::GIRDLE:
	{
		return item.is_girdle();
	}
	case EquipmentSlot::CLOAK:
	{
		return item.get_name().find("cloak") != std::string::npos;
	}
	case EquipmentSlot::RIGHT_HAND:
	{
		return item.is_weapon() && !item.is_ranged_weapon();
	}
	case EquipmentSlot::LEFT_HAND:
	{
		return item.is_shield() || (item.is_weapon() && !item.is_ranged_weapon());
	}
	case EquipmentSlot::RIGHT_RING:
	case EquipmentSlot::LEFT_RING:
	{
		return item.is_ring();
	}
	case EquipmentSlot::BRACERS:
	{
		return item.get_name().find("bracer") != std::string::npos;
	}
	case EquipmentSlot::GAUNTLETS:
	{
		return item.is_gauntlets() && item.get_name().find("cloak") == std::string::npos && item.get_name().find("boot") == std::string::npos && item.get_name().find("bracer") == std::string::npos;
	}
	case EquipmentSlot::BOOTS:
	{
		return item.get_name().find("boot") != std::string::npos;
	}
	case EquipmentSlot::MISSILE_WEAPON:
	{
		return item.is_ranged_weapon();
	}
	case EquipmentSlot::MISSILES:
	{
		return false;
	}
	case EquipmentSlot::TOOL:
	{
		return item.is_tool();
	}
	default:
	{
		return false;
	}
	}
}

bool InventoryUI::is_usable_category(ItemCategory cat) const
{
	return std::ranges::contains(USABLE_CATEGORIES, cat);
}

ItemCategory InventoryUI::get_effective_category(const Item& item) const
{
	if (item.behavior && std::holds_alternative<CorpseFood>(*item.behavior))
	{
		return ItemCategory::CONSUMABLE;
	}
	return item.get_category();
}

// ============================================================
// Rendering
// ============================================================

void InventoryUI::render_tab_bar(GameContext& ctx)
{
	int tileSize = ctx.renderer->get_tile_size();
	int fontSize = ctx.renderer->get_font_size();
	int fontOff = (tileSize - fontSize) / 2;

	// Draw overloaded warning if inventory exceeds max weight
	if (InventoryOperations::is_overloaded(playerRef, *ctx.dataManager))
	{
		std::string_view warning = "OVERLOADED! Movement speed reduced.";
		int warningW = ctx.renderer->measure_text(warning);
		int warningX = (screen_cols(ctx) * tileSize - warningW) / 2;
		ctx.renderer->draw_text(Vector2D{ warningX, fontOff }, warning, RED_BLACK_PAIR);
	}

	int tabY = row_top_y(tileSize, TAB_ROW);
	int px = 2 * tileSize; // start 2 tiles from left

	struct TabLabel
	{
		InventoryScreen screen;
		const char* text;
	};

	const TabLabel tabs[] = {
		{ InventoryScreen::EQUIPMENT, "Equipment" },
		{ InventoryScreen::BACKPACK, "Backpack" },
		{ InventoryScreen::USABLES, "Usables" },
	};

	for (const auto& tab : tabs)
	{
		int colorPair = (tab.screen == activeScreen) ? BLACK_WHITE_PAIR : WHITE_BLACK_PAIR;
		int textW = ctx.renderer->measure_text(tab.text);

		if (tab.screen == activeScreen)
		{
			ColorPair pair = ctx.renderer->get_color_pair(BLACK_WHITE_PAIR);
			DrawRectangle(px - 4, tabY, textW + 8, UI_TEXT_ROW_PITCH, pair.bg);
		}

		ctx.renderer->draw_text(Vector2D{ px, row_text_y(tileSize, fontSize, TAB_ROW) }, tab.text, colorPair);
		px += textW + tileSize; // one-tile gap between tabs
	}

	// Measured back from the panel's inner edge, with clearance for the frame's
	// own rule. Counting tile columns ran the last glyph into the border.
	std::string_view hint = "[Left/Right] Switch";
	int hintW = ctx.renderer->measure_text(hint);
	int hintX = ctx.renderer->get_screen_width() - tileSize - hintW - PANEL_EDGE_CLEARANCE;
	ctx.renderer->draw_text(Vector2D{ hintX, row_text_y(tileSize, fontSize, TAB_ROW) }, hint, CYAN_BLACK_PAIR);
}

void InventoryUI::render_equipment_screen(const Player& player, GameContext& ctx)
{
	int tileSize = ctx.renderer->get_tile_size();
	int fontSize = ctx.renderer->get_font_size();

	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		const auto& slotInfo = SLOT_TABLE[i];
		int y = FIRST_CONTENT_ROW + i;
		bool isCursorRow = (i == equipmentCursor);

		if (isCursorRow)
		{
			draw_highlight_row(y, ctx);
		}

		int rowColor = isCursorRow ? BLACK_WHITE_PAIR : WHITE_BLACK_PAIR;

		std::string slotLabel = std::format("{:<14}: ", slotInfo.label);

		Item* equipped = player.get_equipped_item(slotInfo.slot);
		std::string line;
		if (equipped)
		{
			line = slotLabel + std::string(equipped->get_name());

			std::string stats;
			if (equipped->is_weapon())
			{
				stats = format_weapon_info(*equipped);
			}
			else if (equipped->is_armor() || equipped->is_shield())
			{
				stats = format_armor_info(*equipped);
			}
			if (!stats.empty())
			{
				line += " " + stats;
			}
			if (equipped->get_value() > 0)
			{
				line += std::format(" ({} gp)", equipped->get_value());
			}
		}
		else
		{
			line = slotLabel + "(empty)";
		}

		ctx.renderer->draw_text(Vector2D{ 3 * tileSize, row_text_y(tileSize, fontSize, y) }, line, rowColor);
	}

	if (filterMode)
	{
		int filterY = FIRST_CONTENT_ROW + SLOT_COUNT + 1;
		std::string filterText = std::format("FILTER: {}", SLOT_TABLE[equipmentCursor].label);
		ctx.renderer->draw_text(Vector2D{ 3 * tileSize, row_text_y(tileSize, fontSize, filterY) }, filterText, YELLOW_BLACK_PAIR);
	}
}

void InventoryUI::render_item_list_screen(GameContext& ctx)
{
	int tileSize = ctx.renderer->get_tile_size();
	int fontSize = ctx.renderer->get_font_size();
	int startY = FIRST_CONTENT_ROW;
	int contentHeight = list_content_rows(*ctx.renderer);

	if (listEntries.empty())
	{
		const char* msg = "Your backpack is empty.";
		if (activeScreen == InventoryScreen::USABLES)
		{
			msg = "No usable items.";
		}
		else if (filterMode)
		{
			msg = "No items fit this slot.";
		}
		ctx.renderer->draw_text(Vector2D{ 3 * tileSize, row_text_y(tileSize, fontSize, startY + 1) }, msg, WHITE_BLACK_PAIR);
		return;
	}

	char nextLetter = 'a';
	int y = startY;

	for (int i = scrollOffset; i < static_cast<int>(listEntries.size()) && (y - startY) < contentHeight; ++i)
	{
		const auto& entry = listEntries[i];
		bool isCursorRow = (i == listCursor);

		if (isCursorRow)
		{
			draw_highlight_row(y, ctx);
		}

		if (entry.kind == BackpackEntry::Kind::CATEGORY_HEADER)
		{
			int headerColor = isCursorRow ? BLACK_WHITE_PAIR : YELLOW_BLACK_PAIR;
			ctx.renderer->draw_text(Vector2D{ 3 * tileSize, row_text_y(tileSize, fontSize, y) }, entry.headerText, headerColor);
		}
		else if (entry.item)
		{
			char letter = (nextLetter <= 'z') ? nextLetter++ : ' ';
			std::string line = std::format("{}) {}", letter, std::string(entry.item->get_name()));

			std::string stats;
			if (entry.item->is_weapon())
			{
				stats = format_weapon_info(*entry.item);
			}
			else if (entry.item->is_armor() || entry.item->is_shield())
			{
				stats = format_armor_info(*entry.item);
			}
			if (!stats.empty())
			{
				line += " " + stats;
			}

			if (entry.item->get_value() > 0)
			{
				line += std::format(" ({} gp)", entry.item->get_value());
			}

			int itemColor = isCursorRow ? BLACK_WHITE_PAIR : entry.item->actorData.color;
			ctx.renderer->draw_text(Vector2D{ 3 * tileSize, row_text_y(tileSize, fontSize, y) }, line, itemColor);
		}

		y++;
	}

	int totalEntries = static_cast<int>(listEntries.size());
	int arrowX = screen_cols(ctx) * tileSize - 4 * tileSize;
	if (scrollOffset > 0)
	{
		ctx.renderer->draw_text(Vector2D{ arrowX, row_text_y(tileSize, fontSize, startY) }, "^^^", CYAN_BLACK_PAIR);
	}
	if (scrollOffset + contentHeight < totalEntries)
	{
		ctx.renderer->draw_text(Vector2D{ arrowX, row_text_y(tileSize, fontSize, startY + contentHeight - 1) }, "vvv", CYAN_BLACK_PAIR);
	}
}

void InventoryUI::render_detail_bar(const Player& player, GameContext& ctx)
{
	int tileSize = ctx.renderer->get_tile_size();
	int fontSize = ctx.renderer->get_font_size();
	int detailY = detail_bar_top_row(*ctx.renderer);

	Item* selectedItem = get_selected_item();

	if (!selectedItem && activeScreen == InventoryScreen::EQUIPMENT)
	{
		selectedItem = player.get_equipped_item(SLOT_TABLE[equipmentCursor].slot);
	}

	if (selectedItem)
	{
		std::string nameLine = std::string(selectedItem->get_name());

		std::string primaryStat;
		if (selectedItem->is_weapon())
		{
			primaryStat = format_weapon_info(*selectedItem);
		}
		else if (selectedItem->is_armor() || selectedItem->is_shield())
		{
			primaryStat = format_armor_info(*selectedItem);
		}

		if (!primaryStat.empty())
		{
			nameLine += "  " + primaryStat;
		}

		std::string valueStr = format_value_info(*selectedItem);
		if (!valueStr.empty())
		{
			nameLine += "  " + valueStr;
		}

		ctx.renderer->draw_text(Vector2D{ tileSize, row_text_y(tileSize, fontSize, detailY) }, nameLine, selectedItem->actorData.color);

		std::string enhStr = format_enhancement_info(*selectedItem);
		std::string statStr = format_stat_bonus_info(*selectedItem);
		std::string line2;
		if (!enhStr.empty())
		{
			line2 = enhStr;
		}
		if (!statStr.empty())
		{
			if (!line2.empty())
			{
				line2 += "  ";
			}
			line2 += statStr;
		}
		if (!line2.empty())
		{
			ctx.renderer->draw_text(Vector2D{ tileSize, row_text_y(tileSize, fontSize, detailY + 1) }, line2, WHITE_BLACK_PAIR);
		}
	}
	else if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		ctx.renderer->draw_text(Vector2D{ tileSize, row_text_y(tileSize, fontSize, detailY) }, "Press [Enter] to browse items for this slot.", WHITE_BLACK_PAIR);
	}

	const char* keybinds = (activeScreen == InventoryScreen::EQUIPMENT)
		? "[Enter] Unequip/Browse  [d] Drop  [Left/Right] Switch  [ESC] Close"
		: "[Enter] Use/Equip  [d] Drop  [a-z] Quick Use  [Left/Right] Switch  [ESC] Close";

	// The keybind line is the widest thing on the screen, so it starts one tile in
	// and is cut to what the panel actually holds rather than running under the frame.
	const int keybindWidth = ctx.renderer->get_screen_width() - 2 * tileSize - PANEL_EDGE_CLEARANCE;
	ctx.renderer->draw_text(
		Vector2D{ tileSize, row_text_y(tileSize, fontSize, detailY + DETAIL_BAR_ROWS - 1) },
		ctx.renderer->fit_text_to_width(keybinds, keybindWidth),
		CYAN_BLACK_PAIR);
}

// ============================================================
// Format Helpers
// ============================================================

std::string InventoryUI::format_weapon_info(const Item& item) const
{
	DamageInfo damage = WeaponDamageRegistry::get_enhanced_damage_info(
		item.itemKey,
		&item.get_enhancement());
	const Weapon* weapon = item.behavior ? std::get_if<Weapon>(&*item.behavior) : nullptr;
	bool ranged = weapon && weapon->is_ranged();
	return std::format("[{} {}]", damage.displayRoll, ranged ? "rng" : "dmg");
}

std::string InventoryUI::format_armor_info(const Item& item) const
{
	if (!item.behavior)
		return "";
	int acBonus = get_item_ac_bonus(*item.behavior);
	if (acBonus != 0)
	{
		return std::format("[AC {}]", acBonus);
	}
	return "";
}

std::string InventoryUI::format_stat_bonus_info(const Item& item) const
{
	if (!item.behavior)
		return "";

	// Extract stat bonuses from whichever stat-boost type is active
	int str_b = 0, dex_b = 0, con_b = 0, int_b = 0, wis_b = 0, cha_b = 0;
	bool has_stats = false;

	auto extract_stats = [&](const auto& sb) -> bool
	{
		using T = std::decay_t<decltype(sb)>;
		if constexpr (std::is_same_v<T, JewelryAmulet> || std::is_same_v<T, Gauntlets> || std::is_same_v<T, Girdle>)
		{
			str_b = sb.strBonus;
			dex_b = sb.dexBonus;
			con_b = sb.conBonus;
			int_b = sb.intBonus;
			wis_b = sb.wisBonus;
			cha_b = sb.chaBonus;
			return true;
		}
		return false;
	};

	has_stats = std::visit(extract_stats, *item.behavior);

	if (!has_stats)
		return "";

	std::string result;
	auto append_stat = [&result](const char* name, int val)
	{
		if (val != 0)
		{
			if (!result.empty())
				result += " ";
			result += std::format("{} {:+d}", name, val);
		}
	};

	append_stat("STR", str_b);
	append_stat("DEX", dex_b);
	append_stat("CON", con_b);
	append_stat("INT", int_b);
	append_stat("WIS", wis_b);
	append_stat("CHA", cha_b);

	return result;
}

std::string InventoryUI::format_enhancement_info(const Item& item) const
{
	if (!item.is_enhanced())
	{
		return "";
	}

	const auto& enh = item.get_enhancement();
	std::string result;

	if (enh.toHitBonus != 0)
	{
		result += std::format("Hit {:+d}", enh.toHitBonus);
	}
	if (enh.damageBonus != 0)
	{
		if (!result.empty())
		{
			result += " ";
		}
		result += std::format("Dmg {:+d}", enh.damageBonus);
	}
	if (enh.acBonus != 0)
	{
		if (!result.empty())
		{
			result += " ";
		}
		result += std::format("AC {:+d}", enh.acBonus);
	}

	return result;
}

std::string InventoryUI::format_value_info(const Item& item) const
{
	int val = item.get_value();
	if (val > 0)
	{
		return std::format("Value: {} gp", val);
	}
	return "";
}

std::string InventoryUI::get_category_name(ItemCategory cat) const
{
	switch (cat)
	{
	case ItemCategory::WEAPON:
	{
		return "Weapons";
	}
	case ItemCategory::ARMOR:
	{
		return "Armor";
	}
	case ItemCategory::HELMET:
	{
		return "Helmets";
	}
	case ItemCategory::SHIELD:
	{
		return "Shields";
	}
	case ItemCategory::GAUNTLETS:
	{
		return "Gauntlets";
	}
	case ItemCategory::GIRDLE:
	{
		return "Girdles";
	}
	case ItemCategory::JEWELRY:
	{
		return "Jewelry";
	}
	case ItemCategory::CONSUMABLE:
	{
		return "Consumables";
	}
	case ItemCategory::SCROLL:
	{
		return "Scrolls";
	}
	case ItemCategory::TOOL:
	{
		return "Tools";
	}
	case ItemCategory::TREASURE:
	{
		return "Treasure";
	}
	case ItemCategory::QUEST_ITEM:
	{
		return "Quest Items";
	}
	case ItemCategory::UNKNOWN:
	{
		return "Other";
	}
	default:
	{
		return "Other";
	}
	}
}

// ============================================================
// Input Handling
// ============================================================

bool InventoryUI::handle_input(Player& player, GameContext& ctx)
{
	GameKey key = ctx.inputSystem->get_key();

	// Hover -- track mouse position every frame so cursor follows the pointer
	if (ctx.renderer)
	{
		int tileSize = ctx.renderer->get_tile_size();
		::Vector2 rawMouse = GetMousePosition();
		int mouseRow = row_at_y(tileSize, static_cast<int>(rawMouse.y));

		if (activeScreen == InventoryScreen::EQUIPMENT)
		{
			int slotIdx = mouseRow - FIRST_CONTENT_ROW;
			if (slotIdx >= 0 && slotIdx < SLOT_COUNT)
			{
				equipmentCursor = slotIdx;
			}
		}
		else
		{
			int entryIdx = (mouseRow - FIRST_CONTENT_ROW) + scrollOffset;
			if (entryIdx >= 0 && entryIdx < static_cast<int>(listEntries.size())
				&& listEntries[entryIdx].kind == BackpackEntry::Kind::ITEM)
			{
				listCursor = entryIdx;
			}
		}
	}

	switch (key)
	{
	case GameKey::ESCAPE:
	{
		if (filterMode)
		{
			filterMode = false;
			filterSlot = EquipmentSlot::NONE;
			activeScreen = InventoryScreen::EQUIPMENT;
			return true;
		}
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Inventory closed.", true);
		return false;
	}

	case GameKey::TAB:
	case GameKey::RIGHT:
	{
		handle_tab_switch(1);
		return true;
	}

	case GameKey::LEFT:
	{
		handle_tab_switch(-1);
		return true;
	}

	case GameKey::UP:
	case GameKey::W:
	{
		handle_cursor_up();
		return true;
	}

	case GameKey::DOWN:
	case GameKey::S:
	{
		handle_cursor_down(ctx);
		return true;
	}

	case GameKey::ENTER:
	{
		if (activeScreen == InventoryScreen::EQUIPMENT)
		{
			handle_enter_equipment(player, ctx);
		}
		else
		{
			handle_enter_item(player, ctx);
		}
		return true;
	}

	case GameKey::DROP:
	{
		handle_drop(player, ctx);
		return true;
	}

	case GameKey::MOUSE_LEFT:
	{
		assert(ctx.renderer && "InventoryUI::handle_input MOUSE_LEFT called without a renderer");

		int tileSize = ctx.renderer->get_tile_size();
		::Vector2 rawMouse = GetMousePosition();
		int mousePixelX = static_cast<int>(rawMouse.x);
		int mousePixelY = static_cast<int>(rawMouse.y);
		int mouseRow = row_at_y(tileSize, mousePixelY);

		// Click outside the panel = close. The panel is the screen, so this is a
		// pixel test rather than a tile one.
		if (mousePixelX < 0 || mousePixelX >= ctx.renderer->get_screen_width()
			|| mousePixelY < 0 || mousePixelY >= ctx.renderer->get_screen_height())
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Inventory closed.", true);
			return false;
		}

		// Close button [X]
		if (CloseButtonArea(*ctx.renderer, screen_cols(ctx)).contains(mousePixelX, mousePixelY))
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Inventory closed.", true);
			return false;
		}

		// Tab bar (tile row 1)
		if (mouseRow == 1)
		{
			const std::array<const char*, 3> tabLabels{
				"Equipment", "Backpack", "Usables"
			};
			int px = 2 * tileSize;
			for (int i = 0; i < 3; ++i)
			{
				int textW = ctx.renderer->measure_text(tabLabels[static_cast<size_t>(i)]);
				if (mousePixelX >= px - 4 && mousePixelX < px + textW + 4)
				{
					activeScreen = static_cast<InventoryScreen>(i);
					if (filterMode && activeScreen == InventoryScreen::EQUIPMENT)
					{
						filterMode = false;
						filterSlot = EquipmentSlot::NONE;
					}
					listCursor = 0;
					scrollOffset = 0;
					return true;
				}
				px += textW + tileSize;
			}
			return true;
		}

		const int detailY = detail_bar_top_row(*ctx.renderer);

		if (mouseRow < FIRST_CONTENT_ROW || mouseRow >= detailY)
		{
			return true;
		}

		if (activeScreen == InventoryScreen::EQUIPMENT)
		{
			int slotIdx = mouseRow - FIRST_CONTENT_ROW;
			if (slotIdx >= 0 && slotIdx < SLOT_COUNT)
			{
				equipmentCursor = slotIdx;
				handle_enter_equipment(player, ctx);
			}
		}
		else
		{
			int entryIdx = (mouseRow - FIRST_CONTENT_ROW) + scrollOffset;
			if (entryIdx >= 0 && entryIdx < static_cast<int>(listEntries.size()))
			{
				listCursor = entryIdx;
				if (listEntries[entryIdx].kind == BackpackEntry::Kind::ITEM)
				{
					handle_enter_item(player, ctx);
				}
			}
		}

		return true;
	}

	default:
	{
		// Letter shortcuts for Backpack/Usables screens
		int charInput = ctx.inputSystem->get_char_input();
		if (activeScreen != InventoryScreen::EQUIPMENT && charInput >= 'a' && charInput <= 'z')
		{
			int letterIndex = charInput - 'a';
			int itemCount = 0;
			for (int i = 0; i < static_cast<int>(listEntries.size()); ++i)
			{
				if (listEntries[i].kind == BackpackEntry::Kind::ITEM)
				{
					if (itemCount == letterIndex)
					{
						listCursor = i;
						handle_enter_item(player, ctx);
						return true;
					}
					itemCount++;
				}
			}
		}
		return true;
	}
	}
}

void InventoryUI::handle_cursor_up()
{
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		if (equipmentCursor > 0)
		{
			equipmentCursor--;
		}
	}
	else
	{
		auto next = get_next_item_index(listCursor, -1);
		if (next)
		{
			listCursor = *next;
			if (listCursor < scrollOffset)
			{
				scrollOffset = listCursor;
			}
		}
	}
}

void InventoryUI::handle_cursor_down(GameContext& ctx)
{
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		if (equipmentCursor < SLOT_COUNT - 1)
		{
			equipmentCursor++;
		}
	}
	else
	{
		auto next = get_next_item_index(listCursor, 1);
		if (next)
		{
			listCursor = *next;
			int contentHeight = list_content_rows(*ctx.renderer);
			if (listCursor >= scrollOffset + contentHeight)
			{
				scrollOffset = listCursor - contentHeight + 1;
			}
		}
	}
}

void InventoryUI::handle_tab_switch(int direction)
{
	constexpr int numTabs = 3;
	int current = static_cast<int>(activeScreen);
	current = (current + direction + numTabs) % numTabs;
	activeScreen = static_cast<InventoryScreen>(current);

	if (filterMode && activeScreen == InventoryScreen::EQUIPMENT)
	{
		filterMode = false;
		filterSlot = EquipmentSlot::NONE;
	}

	listCursor = 0;
	scrollOffset = 0;
}

void InventoryUI::handle_enter_equipment(Player& player, GameContext& ctx)
{
	EquipmentSlot slot = SLOT_TABLE[equipmentCursor].slot;
	Item* equipped = player.get_equipped_item(slot);

	if (equipped)
	{
		std::string itemName = std::string(equipped->get_name());
		player.unequip_item(slot, ctx);
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			std::format("You unequipped the {}.", itemName),
			true);
	}
	else
	{
		filterMode = true;
		filterSlot = slot;
		activeScreen = InventoryScreen::BACKPACK;
		listCursor = 0;
		scrollOffset = 0;
	}
}

void InventoryUI::handle_enter_item(Player& player, GameContext& ctx)
{
	if (listCursor < 0 || listCursor >= static_cast<int>(listEntries.size()))
	{
		return;
	}

	const auto& entry = listEntries[listCursor];
	if (entry.kind != BackpackEntry::Kind::ITEM || !entry.item)
	{
		return;
	}

	Item* selectedItem = entry.item;
	if (!selectedItem->behavior)
		return;

	bool itemUsed = use_item(*selectedItem->behavior, *selectedItem, player, ctx);

	if (itemUsed)
	{
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);

		if (filterMode)
		{
			filterMode = false;
			filterSlot = EquipmentSlot::NONE;
			activeScreen = InventoryScreen::EQUIPMENT;
		}
	}
}

void InventoryUI::handle_drop(Player& player, GameContext& ctx)
{
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		EquipmentSlot slot = SLOT_TABLE[equipmentCursor].slot;
		Item* equipped = player.get_equipped_item(slot);
		if (!equipped)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Nothing to drop.", true);
			return;
		}

		std::string itemName = std::string(equipped->get_name());
		uint64_t itemId = equipped->uniqueId;
		player.unequip_item(slot, ctx);

		auto find_by_id = [itemId](const std::unique_ptr<Item>& it)
		{
			return it && it->uniqueId == itemId;
		};
		auto it = std::ranges::find_if(player.inventoryData.items, find_by_id);
		if (it != player.inventoryData.items.end())
		{
			player.drop(**it, ctx);
			ctx.messageSystem->message(
				WHITE_BLACK_PAIR,
				std::format("You drop the {}.", itemName),
				true);
		}
	}
	else
	{
		if (listCursor < 0 || listCursor >= static_cast<int>(listEntries.size()))
		{
			return;
		}

		const auto& entry = listEntries[listCursor];
		if (entry.kind != BackpackEntry::Kind::ITEM || !entry.item)
		{
			return;
		}

		std::string itemName = std::string(entry.item->get_name());
		player.drop(*entry.item, ctx);
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			std::format("You drop the {}.", itemName),
			true);
	}
}

// ============================================================
// Helpers
// ============================================================

std::optional<int> InventoryUI::get_next_item_index(int from, int direction) const
{
	int idx = from + direction;
	while (idx >= 0 && idx < static_cast<int>(listEntries.size()))
	{
		if (listEntries[idx].kind == BackpackEntry::Kind::ITEM)
		{
			return idx;
		}
		idx += direction;
	}
	return std::nullopt;
}

Item* InventoryUI::get_selected_item() const
{
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		return nullptr;
	}

	if (listCursor >= 0 && listCursor < static_cast<int>(listEntries.size()) && listEntries[listCursor].kind == BackpackEntry::Kind::ITEM)
	{
		return listEntries[listCursor].item;
	}

	return nullptr;
}
