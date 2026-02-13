#include <algorithm>
#include <format>
#include <ranges>

#include "InventoryUI.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Pickable.h"
#include "../Actor/InventoryOperations.h"
#include "../Controls/Controls.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Items/Jewelry.h"
#include "../Items/Armor.h"

using namespace InventoryOperations;

InventoryUI::InventoryUI()
	: activeScreen(InventoryScreen::EQUIPMENT)
	, equipmentCursor(0)
	, listCursor(0)
	, scrollOffset(0)
	, filterMode(false)
	, filterSlot(EquipmentSlot::NONE)
{
}

void InventoryUI::display(Player& player, GameContext& ctx, InventoryScreen startScreen)
{
	activeScreen = startScreen;
	equipmentCursor = 0;
	listCursor = 0;
	scrollOffset = 0;
	filterMode = false;
	filterSlot = EquipmentSlot::NONE;

	rebuild_item_list(player);

	while (true)
	{
		ctx.renderer->begin_frame();

		render_tab_bar(ctx);

		if (activeScreen == InventoryScreen::EQUIPMENT)
		{
			render_equipment_screen(player, ctx);
		}
		else
		{
			render_item_list_screen(ctx);
		}

		render_detail_bar(player, ctx);

		ctx.renderer->end_frame();

		ctx.input_system->poll();
		if (!handle_input(player, ctx))
		{
			break;
		}

		rebuild_item_list(player);
	}
}

// ============================================================
// Data Building
// ============================================================

void InventoryUI::rebuild_item_list(const Player& player)
{
	listEntries.clear();

	std::vector<Item*> items;
	for (const auto& item : player.inventory_data.items)
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
		header.header_text = std::format("-- {} --", get_category_name(cat));
		header.item = nullptr;
		listEntries.push_back(std::move(header));

		for (Item* it : catItems)
		{
			BackpackEntry entry;
			entry.kind = BackpackEntry::Kind::ITEM;
			entry.category = cat;
			entry.header_text.clear();
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
			int next = get_next_item_index(listCursor, 1);
			if (next >= 0)
			{
				listCursor = next;
			}
		}
	}

	int contentHeight = SCREEN_ROWS - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;
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
		return item.is_helmet();
	case EquipmentSlot::NECK:
		return item.is_amulet();
	case EquipmentSlot::BODY:
		return item.is_armor();
	case EquipmentSlot::GIRDLE:
		return item.is_girdle();
	case EquipmentSlot::CLOAK:
		return item.get_name().find("cloak") != std::string::npos;
	case EquipmentSlot::RIGHT_HAND:
		return item.is_weapon() && !item.is_ranged_weapon();
	case EquipmentSlot::LEFT_HAND:
		return item.is_shield() || (item.is_weapon() && !item.is_ranged_weapon());
	case EquipmentSlot::RIGHT_RING:
	case EquipmentSlot::LEFT_RING:
		return item.is_ring();
	case EquipmentSlot::BRACERS:
		return item.get_name().find("bracer") != std::string::npos;
	case EquipmentSlot::GAUNTLETS:
		return item.is_gauntlets()
			&& item.get_name().find("cloak") == std::string::npos
			&& item.get_name().find("boot") == std::string::npos
			&& item.get_name().find("bracer") == std::string::npos;
	case EquipmentSlot::BOOTS:
		return item.get_name().find("boot") != std::string::npos;
	case EquipmentSlot::MISSILE_WEAPON:
		return item.is_ranged_weapon();
	case EquipmentSlot::MISSILES:
		return false;
	case EquipmentSlot::TOOL:
		return item.is_tool();
	default:
		return false;
	}
}

bool InventoryUI::is_usable_category(ItemCategory cat) const
{
	return std::ranges::find(USABLE_CATEGORIES, cat) != USABLE_CATEGORIES.end();
}

ItemCategory InventoryUI::get_effective_category(const Item& item) const
{
	if (item.pickable
		&& item.pickable->get_type() == Pickable::PickableType::CORPSE_FOOD)
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
	int ts = ctx.renderer->get_tile_size();
	int x = 2;

	struct TabLabel
	{
		InventoryScreen screen;
		const char* text;
	};

	const TabLabel tabs[] =
	{
		{InventoryScreen::EQUIPMENT, " Equipment "},
		{InventoryScreen::BACKPACK,  " Backpack "},
		{InventoryScreen::USABLES,   " Usables "},
	};

	for (const auto& tab : tabs)
	{
		int colorPair = (tab.screen == activeScreen) ? BLACK_WHITE_PAIR : WHITE_BLACK_PAIR;
		ctx.renderer->draw_text(x * ts, 0, tab.text, colorPair);
		x += static_cast<int>(strlen(tab.text)) + 1;
	}

	ctx.renderer->draw_text((SCREEN_COLS - 14) * ts, 0, "[Tab] Switch", CYAN_BLACK_PAIR);
}

void InventoryUI::render_equipment_screen(const Player& player, GameContext& ctx)
{
	int ts = ctx.renderer->get_tile_size();
	int startY = 1 + TAB_BAR_HEIGHT;

	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		const auto& slotInfo = SLOT_TABLE[i];
		int y = startY + i;
		bool isCursorRow = (i == equipmentCursor);

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
			if (equipped->value > 0)
			{
				line += std::format(" ({} gp)", equipped->value);
			}
		}
		else
		{
			line = slotLabel + "(empty)";
		}

		ctx.renderer->draw_text(3 * ts, y * ts, line, rowColor);
	}

	if (filterMode)
	{
		int filterY = startY + SLOT_COUNT + 1;
		std::string filterText = std::format("FILTER: {}", SLOT_TABLE[equipmentCursor].label);
		ctx.renderer->draw_text(3 * ts, filterY * ts, filterText, YELLOW_BLACK_PAIR);
	}
}

void InventoryUI::render_item_list_screen(GameContext& ctx)
{
	int ts = ctx.renderer->get_tile_size();
	int startY = 1 + TAB_BAR_HEIGHT;
	int contentHeight = SCREEN_ROWS - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;

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
		ctx.renderer->draw_text(3 * ts, (startY + 1) * ts, msg, WHITE_BLACK_PAIR);
		return;
	}

	char nextLetter = 'a';
	int y = startY;

	for (int i = scrollOffset; i < static_cast<int>(listEntries.size()) && (y - startY) < contentHeight; ++i)
	{
		const auto& entry = listEntries[i];
		bool isCursorRow = (i == listCursor);

		int rowColor = isCursorRow ? BLACK_WHITE_PAIR : WHITE_BLACK_PAIR;

		if (entry.kind == BackpackEntry::Kind::CATEGORY_HEADER)
		{
			int headerColor = isCursorRow ? BLACK_WHITE_PAIR : YELLOW_BLACK_PAIR;
			ctx.renderer->draw_text(3 * ts, y * ts, entry.header_text, headerColor);
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

			if (entry.item->value > 0)
			{
				line += std::format(" ({} gp)", entry.item->value);
			}

			int itemColor = isCursorRow ? BLACK_WHITE_PAIR : entry.item->actorData.color;
			ctx.renderer->draw_text(3 * ts, y * ts, line, itemColor);
		}

		y++;
	}

	int totalEntries = static_cast<int>(listEntries.size());
	if (scrollOffset > 0)
	{
		ctx.renderer->draw_text((SCREEN_COLS - 5) * ts, startY * ts, "^^^", CYAN_BLACK_PAIR);
	}
	if (scrollOffset + contentHeight < totalEntries)
	{
		ctx.renderer->draw_text((SCREEN_COLS - 5) * ts, (startY + contentHeight - 1) * ts, "vvv", CYAN_BLACK_PAIR);
	}
}

void InventoryUI::render_detail_bar(const Player& player, GameContext& ctx)
{
	int ts = ctx.renderer->get_tile_size();
	int detailY = SCREEN_ROWS - DETAIL_BAR_HEIGHT;

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

		ctx.renderer->draw_text(2 * ts, (detailY + 1) * ts, nameLine, selectedItem->actorData.color);

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
			ctx.renderer->draw_text(2 * ts, (detailY + 2) * ts, line2, WHITE_BLACK_PAIR);
		}
	}
	else if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		ctx.renderer->draw_text(2 * ts, (detailY + 1) * ts, "Press [Enter] to browse items for this slot.", WHITE_BLACK_PAIR);
	}

	const char* keybinds = (activeScreen == InventoryScreen::EQUIPMENT)
		? "[Enter] Unequip/Browse  [d] Drop  [Tab] Switch  [ESC] Close"
		: "[Enter] Use/Equip  [d] Drop  [a-z] Quick Use  [Tab] Switch  [ESC] Close";

	ctx.renderer->draw_text(2 * ts, (detailY + DETAIL_BAR_HEIGHT - 1) * ts, keybinds, CYAN_BLACK_PAIR);
}

// ============================================================
// Format Helpers
// ============================================================

std::string InventoryUI::format_weapon_info(const Item& item) const
{
	DamageInfo damage = WeaponDamageRegistry::get_enhanced_damage_info(
		item.itemId,
		&item.get_enhancement()
	);
	Weapon* weapon = dynamic_cast<Weapon*>(item.pickable.get());
	bool ranged = weapon && weapon->is_ranged();
	return std::format("[{} {}]", damage.displayRoll, ranged ? "rng" : "dmg");
}

std::string InventoryUI::format_armor_info(const Item& item) const
{
	if (!item.pickable)
	{
		return "";
	}
	int acBonus = item.pickable->get_ac_bonus();
	if (acBonus != 0)
	{
		return std::format("[AC {}]", acBonus);
	}
	return "";
}

std::string InventoryUI::format_stat_bonus_info(const Item& item) const
{
	auto* statEquip = dynamic_cast<StatBoostEquipment*>(item.pickable.get());
	if (!statEquip)
	{
		return "";
	}

	std::string result;
	auto append_stat = [&result](const char* name, int val)
	{
		if (val != 0)
		{
			if (!result.empty())
			{
				result += " ";
			}
			result += std::format("{} {:+d}", name, val);
		}
	};

	append_stat("STR", statEquip->str_bonus);
	append_stat("DEX", statEquip->dex_bonus);
	append_stat("CON", statEquip->con_bonus);
	append_stat("INT", statEquip->int_bonus);
	append_stat("WIS", statEquip->wis_bonus);
	append_stat("CHA", statEquip->cha_bonus);

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

	if (enh.to_hit_bonus != 0)
	{
		result += std::format("Hit {:+d}", enh.to_hit_bonus);
	}
	if (enh.damage_bonus != 0)
	{
		if (!result.empty())
		{
			result += " ";
		}
		result += std::format("Dmg {:+d}", enh.damage_bonus);
	}
	if (enh.ac_bonus != 0)
	{
		if (!result.empty())
		{
			result += " ";
		}
		result += std::format("AC {:+d}", enh.ac_bonus);
	}

	return result;
}

std::string InventoryUI::format_value_info(const Item& item) const
{
	int val = item.get_enhanced_value();
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
	case ItemCategory::WEAPON:    return "Weapons";
	case ItemCategory::ARMOR:     return "Armor";
	case ItemCategory::HELMET:    return "Helmets";
	case ItemCategory::SHIELD:    return "Shields";
	case ItemCategory::GAUNTLETS: return "Gauntlets";
	case ItemCategory::GIRDLE:    return "Girdles";
	case ItemCategory::JEWELRY:   return "Jewelry";
	case ItemCategory::CONSUMABLE:return "Consumables";
	case ItemCategory::SCROLL:    return "Scrolls";
	case ItemCategory::TOOL:      return "Tools";
	case ItemCategory::TREASURE:  return "Treasure";
	case ItemCategory::QUEST_ITEM:return "Quest Items";
	case ItemCategory::UNKNOWN:   return "Other";
	default:                      return "Other";
	}
}

// ============================================================
// Input Handling
// ============================================================

bool InventoryUI::handle_input(Player& player, GameContext& ctx)
{
	GameKey key = ctx.input_system->get_key();

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
		ctx.message_system->message(WHITE_BLACK_PAIR, "Inventory closed.", true);
		return false;
	}

	case GameKey::TAB:
		handle_tab_switch();
		return true;

	case GameKey::UP:
	case GameKey::W:
		handle_cursor_up();
		return true;

	case GameKey::DOWN:
	case GameKey::S:
		handle_cursor_down();
		return true;

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

	default:
	{
		// Letter shortcuts for Backpack/Usables screens
		int charInput = ctx.input_system->get_char_input();
		if (activeScreen != InventoryScreen::EQUIPMENT
			&& charInput >= 'a' && charInput <= 'z' && charInput != 'd')
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
		int next = get_next_item_index(listCursor, -1);
		if (next >= 0)
		{
			listCursor = next;
			if (listCursor < scrollOffset)
			{
				scrollOffset = listCursor;
			}
		}
	}
}

void InventoryUI::handle_cursor_down()
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
		int next = get_next_item_index(listCursor, 1);
		if (next >= 0)
		{
			listCursor = next;
			int contentHeight = SCREEN_ROWS - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;
			if (listCursor >= scrollOffset + contentHeight)
			{
				scrollOffset = listCursor - contentHeight + 1;
			}
		}
	}
}

void InventoryUI::handle_tab_switch()
{
	switch (activeScreen)
	{
	case InventoryScreen::EQUIPMENT:
		activeScreen = InventoryScreen::BACKPACK;
		break;
	case InventoryScreen::BACKPACK:
		activeScreen = InventoryScreen::USABLES;
		break;
	case InventoryScreen::USABLES:
		activeScreen = InventoryScreen::EQUIPMENT;
		break;
	}

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
		ctx.message_system->message(
			WHITE_BLACK_PAIR,
			std::format("You unequipped the {}.", itemName),
			true
		);
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
	if (!selectedItem->pickable)
	{
		return;
	}

	bool itemUsed = selectedItem->pickable->use(*selectedItem, player, ctx);

	if (itemUsed)
	{
		*ctx.game_status = GameStatus::NEW_TURN;

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
			ctx.message_system->message(WHITE_BLACK_PAIR, "Nothing to drop.", true);
			return;
		}

		std::string itemName = std::string(equipped->get_name());
		uint64_t itemId = equipped->uniqueId;
		player.unequip_item(slot, ctx);

		auto find_by_id = [itemId](const std::unique_ptr<Item>& it)
		{
			return it && it->uniqueId == itemId;
		};
		auto it = std::ranges::find_if(player.inventory_data.items, find_by_id);
		if (it != player.inventory_data.items.end())
		{
			player.drop(**it, ctx);
			ctx.message_system->message(
				WHITE_BLACK_PAIR,
				std::format("You drop the {}.", itemName),
				true
			);
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
		ctx.message_system->message(
			WHITE_BLACK_PAIR,
			std::format("You drop the {}.", itemName),
			true
		);
	}
}

// ============================================================
// Helpers
// ============================================================

int InventoryUI::get_next_item_index(int from, int direction) const
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
	return -1;
}

Item* InventoryUI::get_selected_item() const
{
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		return nullptr;
	}

	if (listCursor >= 0
		&& listCursor < static_cast<int>(listEntries.size())
		&& listEntries[listCursor].kind == BackpackEntry::Kind::ITEM)
	{
		return listEntries[listCursor].item;
	}

	return nullptr;
}
