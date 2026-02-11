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
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Items/Jewelry.h"
#include "../Items/Armor.h"

using namespace InventoryOperations;

InventoryUI::InventoryUI()
	: mainWindow(nullptr)
	, detailWindow(nullptr)
	, activeScreen(InventoryScreen::EQUIPMENT)
	, equipmentCursor(0)
	, listCursor(0)
	, scrollOffset(0)
	, filterMode(false)
	, filterSlot(EquipmentSlot::NONE)
{
}

InventoryUI::~InventoryUI()
{
	destroy_windows();
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
	create_windows();

	while (true)
	{
		redraw_frame();

		if (activeScreen == InventoryScreen::EQUIPMENT)
		{
			render_equipment_screen(player);
		}
		else
		{
			render_item_list_screen();
		}

		render_tab_bar();
		render_detail_bar(player, ctx);
		refresh_windows();

		if (!handle_input(player, ctx))
		{
			break;
		}

		rebuild_item_list(player);
	}

	destroy_windows();
	restore_game_display(ctx);
}

// ============================================================
// Window Management
// ============================================================

void InventoryUI::create_windows()
{
	clear();
	refresh();

	int mainHeight = LINES - DETAIL_BAR_HEIGHT;
	mainWindow = newwin(mainHeight, COLS, 0, 0);
	detailWindow = newwin(DETAIL_BAR_HEIGHT, COLS, mainHeight, 0);

	keypad(mainWindow, TRUE);
}

void InventoryUI::redraw_frame()
{
	werase(mainWindow);
	werase(detailWindow);

	box(mainWindow, 0, 0);
	box(detailWindow, 0, 0);
}

void InventoryUI::destroy_windows()
{
	if (mainWindow)
	{
		delwin(mainWindow);
		mainWindow = nullptr;
	}
	if (detailWindow)
	{
		delwin(detailWindow);
		detailWindow = nullptr;
	}
}

void InventoryUI::refresh_windows()
{
	wnoutrefresh(mainWindow);
	wnoutrefresh(detailWindow);
	doupdate();
}

// ============================================================
// Data Building
// ============================================================

void InventoryUI::rebuild_item_list(const Player& player)
{
	listEntries.clear();

	// Collect non-null, non-equipped items from inventory
	std::vector<Item*> items;
	for (const auto& item : player.inventory_data.items)
	{
		if (!item)
		{
			continue;
		}

		// Exclude equipped items
		auto is_equipped = [&item](const EquippedItem& eq)
		{
			return eq.item && eq.item->uniqueId == item->uniqueId;
		};
		if (std::ranges::any_of(player.equippedItems, is_equipped))
		{
			continue;
		}

		// Filter mode: only items fitting the slot
		if (filterMode && !item_fits_slot(*item, filterSlot))
		{
			continue;
		}

		ItemCategory effectiveCat = get_effective_category(*item);

		// Usables tab: only show usable categories
		if (activeScreen == InventoryScreen::USABLES && !is_usable_category(effectiveCat))
		{
			continue;
		}

		items.push_back(item.get());
	}

	// Determine which category list to use
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

	// Clamp cursor and skip headers
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

	// Clamp scroll offset
	int contentHeight = LINES - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;
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

void InventoryUI::render_tab_bar()
{
	int y = 0;
	int x = 2;

	// Draw tab labels on the top border of mainWindow
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
		if (tab.screen == activeScreen)
		{
			wattron(mainWindow, A_BOLD | A_REVERSE);
			mvwprintw(mainWindow, y, x, "%s", tab.text);
			wattroff(mainWindow, A_BOLD | A_REVERSE);
		}
		else
		{
			wattron(mainWindow, A_DIM);
			mvwprintw(mainWindow, y, x, "%s", tab.text);
			wattroff(mainWindow, A_DIM);
		}
		x += static_cast<int>(strlen(tab.text)) + 1;
	}

	// Tab key hint on the right
	wattron(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
	mvwprintw(mainWindow, y, COLS - 14, "[Tab] Switch");
	wattroff(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
}

void InventoryUI::render_equipment_screen(const Player& player)
{
	int startY = 1 + TAB_BAR_HEIGHT;
	int contentWidth = COLS - 4;

	for (int i = 0; i < SLOT_COUNT; ++i)
	{
		const auto& slotInfo = SLOT_TABLE[i];
		int y = startY + i;
		bool isCursorRow = (i == equipmentCursor);

		if (isCursorRow)
		{
			wattron(mainWindow, A_REVERSE);
		}

		// Clear the row
		mvwprintw(mainWindow, y, 1, "%-*s", contentWidth, "");

		// Slot label
		mvwprintw(mainWindow, y, 3, "%-14s: ", slotInfo.label);

		Item* equipped = player.get_equipped_item(slotInfo.slot);
		if (equipped)
		{
			if (!isCursorRow)
			{
				wattron(mainWindow, COLOR_PAIR(equipped->actorData.color));
			}

			std::string name = std::string(equipped->get_name());
			wprintw(mainWindow, "%s", name.c_str());

			if (!isCursorRow)
			{
				wattroff(mainWindow, COLOR_PAIR(equipped->actorData.color));
			}

			// Inline stats after the name
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
				wprintw(mainWindow, " %s", stats.c_str());
			}

			if (equipped->value > 0)
			{
				if (!isCursorRow)
				{
					wattron(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
				}
				wprintw(mainWindow, " (%d gp)", equipped->value);
				if (!isCursorRow)
				{
					wattroff(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
				}
			}
		}
		else
		{
			if (!isCursorRow)
			{
				wattron(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
			}
			wprintw(mainWindow, "(empty)");
			if (!isCursorRow)
			{
				wattroff(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
			}
		}

		if (isCursorRow)
		{
			wattroff(mainWindow, A_REVERSE);
		}
	}

	// Filter mode indicator
	if (filterMode)
	{
		int filterY = startY + SLOT_COUNT + 1;
		wattron(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR) | A_BOLD);
		mvwprintw(mainWindow, filterY, 3, "FILTER: %s", SLOT_TABLE[equipmentCursor].label);
		wattroff(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR) | A_BOLD);
	}
}

void InventoryUI::render_item_list_screen()
{
	int startY = 1 + TAB_BAR_HEIGHT;
	int contentHeight = LINES - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;
	int contentWidth = COLS - 4;

	if (listEntries.empty())
	{
		if (activeScreen == InventoryScreen::USABLES)
		{
			mvwprintw(mainWindow, startY + 1, 3, "No usable items.");
		}
		else if (filterMode)
		{
			mvwprintw(mainWindow, startY + 1, 3, "No items fit this slot.");
		}
		else
		{
			mvwprintw(mainWindow, startY + 1, 3, "Your backpack is empty.");
		}
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
			wattron(mainWindow, A_REVERSE);
		}

		// Clear row
		mvwprintw(mainWindow, y, 1, "%-*s", contentWidth, "");

		if (entry.kind == BackpackEntry::Kind::CATEGORY_HEADER)
		{
			if (!isCursorRow)
			{
				wattron(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR) | A_BOLD);
			}
			mvwprintw(mainWindow, y, 3, "%s", entry.header_text.c_str());
			if (!isCursorRow)
			{
				wattroff(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR) | A_BOLD);
			}
		}
		else if (entry.item)
		{
			char letter = (nextLetter <= 'z') ? nextLetter++ : ' ';
			mvwprintw(mainWindow, y, 3, "%c) ", letter);

			if (!isCursorRow)
			{
				wattron(mainWindow, COLOR_PAIR(entry.item->actorData.color));
			}
			wprintw(mainWindow, "%s", std::string(entry.item->get_name()).c_str());
			if (!isCursorRow)
			{
				wattroff(mainWindow, COLOR_PAIR(entry.item->actorData.color));
			}

			// Inline stats
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
				wprintw(mainWindow, " %s", stats.c_str());
			}

			if (entry.item->value > 0)
			{
				if (!isCursorRow)
				{
					wattron(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
				}
				wprintw(mainWindow, " (%d gp)", entry.item->value);
				if (!isCursorRow)
				{
					wattroff(mainWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
				}
			}
		}

		if (isCursorRow)
		{
			wattroff(mainWindow, A_REVERSE);
		}

		y++;
	}

	// Scroll indicators
	int totalEntries = static_cast<int>(listEntries.size());
	if (scrollOffset > 0)
	{
		wattron(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
		mvwprintw(mainWindow, startY, contentWidth - 2, "^^^");
		wattroff(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
	}
	if (scrollOffset + contentHeight < totalEntries)
	{
		wattron(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
		mvwprintw(mainWindow, startY + contentHeight - 1, contentWidth - 2, "vvv");
		wattroff(mainWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
	}
}

void InventoryUI::render_detail_bar(const Player& player, GameContext& ctx)
{
	Item* selectedItem = get_selected_item();

	if (!selectedItem && activeScreen == InventoryScreen::EQUIPMENT)
	{
		selectedItem = player.get_equipped_item(SLOT_TABLE[equipmentCursor].slot);
	}

	if (selectedItem)
	{
		// Line 1: Item name + primary stat + value
		wattron(detailWindow, A_BOLD | COLOR_PAIR(selectedItem->actorData.color));
		mvwprintw(detailWindow, 1, 2, "%s", std::string(selectedItem->get_name()).c_str());
		wattroff(detailWindow, A_BOLD | COLOR_PAIR(selectedItem->actorData.color));

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
			wprintw(detailWindow, "  %s", primaryStat.c_str());
		}

		std::string valueStr = format_value_info(*selectedItem);
		if (!valueStr.empty())
		{
			wprintw(detailWindow, "  %s", valueStr.c_str());
		}

		// Line 2: Enhancement and stat bonuses
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
			mvwprintw(detailWindow, 2, 2, "%s", line2.c_str());
		}
	}
	else if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		mvwprintw(detailWindow, 1, 2, "Press [Enter] to browse items for this slot.");
	}

	// Keybinds help
	wattron(detailWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
	if (activeScreen == InventoryScreen::EQUIPMENT)
	{
		mvwprintw(
			detailWindow,
			DETAIL_BAR_HEIGHT - 2,
			2,
			"[Enter] Unequip/Browse  [d] Drop  [Tab] Switch  [ESC] Close"
		);
	}
	else
	{
		mvwprintw(
			detailWindow,
			DETAIL_BAR_HEIGHT - 2,
			2,
			"[Enter] Use/Equip  [d] Drop  [a-z] Quick Use  [Tab] Switch  [ESC] Close"
		);
	}
	wattroff(detailWindow, COLOR_PAIR(CYAN_BLACK_PAIR));
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
	int input = wgetch(mainWindow);

	switch (input)
	{
	case static_cast<int>(Controls::ESCAPE):
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

	case '\t':
		handle_tab_switch();
		return true;

	case KEY_UP:
		handle_cursor_up();
		return true;

	case KEY_DOWN:
		handle_cursor_down();
		return true;

	case '\n':
	case '\r':
	case KEY_ENTER:
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

	case 'd':
	{
		handle_drop(player, ctx);
		return true;
	}

	default:
	{
		// Letter shortcuts for Backpack/Usables screens
		if (activeScreen != InventoryScreen::EQUIPMENT
			&& input >= 'a' && input <= 'z' && input != 'd')
		{
			int letterIndex = input - 'a';
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
			int contentHeight = LINES - DETAIL_BAR_HEIGHT - 2 - TAB_BAR_HEIGHT;
			if (listCursor >= scrollOffset + contentHeight)
			{
				scrollOffset = listCursor - contentHeight + 1;
			}
		}
	}
}

void InventoryUI::handle_tab_switch()
{
	// Cycle: Equipment -> Backpack -> Usables -> Equipment
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

	// Reset filter when switching away
	if (filterMode && activeScreen == InventoryScreen::EQUIPMENT)
	{
		filterMode = false;
		filterSlot = EquipmentSlot::NONE;
	}

	// Reset list cursor for the new screen
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
		// Enter filter mode on the Backpack tab
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

	Item* itemPtr = selectedItem;
	bool itemUsed = selectedItem->pickable->use(*selectedItem, player, ctx);

	if (itemUsed)
	{
		*ctx.game_status = GameStatus::NEW_TURN;

		// Exit filter mode after equipping
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

void InventoryUI::restore_game_display(GameContext& ctx)
{
	clear();
	refresh();
	ctx.rendering_manager->restore_game_display();
}
