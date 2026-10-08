#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "BaseMenu.h"
#include "EquipmentSlot.h"
#include "ItemClassification.h"

class Creature;
class Item;
class Player;
struct GameContext;
class Renderer;

enum class InventoryScreen
{
	EQUIPMENT,
	BACKPACK,
	USABLES
};

// What a right-click offers on an inventory row.
//
// The labels only. Every one of them is something the keyboard already does, so the
// menu moves the cursor to the clicked row and calls the same handler rather than
// carrying a second copy of what the action means.
namespace InventoryActions
{

// The labels for a row, in the order shown. Empty when the row has nothing to act on,
// and no menu is opened at all - the rule the map's context menu already follows.
//
// Example:
//   labels_for_row(InventoryScreen::BACKPACK, true);     // -> Use, Drop, Cancel
//   labels_for_row(InventoryScreen::EQUIPMENT, true);    // -> Unequip, Drop, Cancel
//   labels_for_row(InventoryScreen::EQUIPMENT, false);   // -> Browse, Cancel
//   labels_for_row(InventoryScreen::BACKPACK, false);    // -> {}, a category heading
[[nodiscard]] std::vector<std::string> labels_for_row(InventoryScreen screen, bool rowHoldsItem);

// Where one pane tab sits on the tab row.
struct TabBox
{
	InventoryScreen screen{ InventoryScreen::EQUIPMENT };
	const char* text{ "" };
	int x{ 0 };
	int width{ 0 };
};

// The three tabs, left to right. Drawing paints these and the click test reads these,
// so the two cannot disagree about where a tab is - which they did: the hit test looked
// on row 1 while TAB_ROW is 0, and no tab could be clicked at all.
[[nodiscard]] std::array<TabBox, 3> tab_boxes(const Renderer& renderer);

// Which pane the pointer is over, or nothing when it is not on a tab.
//
// Example:
//   tab_at(middleOfTheBackpackTab, tabRowY, renderer);   // -> BACKPACK
//   tab_at(middleOfTheBackpackTab, aContentRowY, r);     // -> nothing, wrong row
[[nodiscard]] std::optional<InventoryScreen> tab_at(int pixelX, int pixelY, const Renderer& renderer);

// The pane after this one, wrapping. Tab steps through the panes with the keyboard the
// way clicking steps through them with the mouse.
//
// Example:
//   next_screen(InventoryScreen::EQUIPMENT);   // -> BACKPACK
//   next_screen(InventoryScreen::USABLES);     // -> EQUIPMENT
[[nodiscard]] InventoryScreen next_screen(InventoryScreen screen);

} // namespace InventoryActions

struct BackpackEntry
{
	enum class Kind
	{
		CATEGORY_HEADER,
		ITEM
	};

	Kind kind{};
	ItemCategory category{};
	std::string headerText{};
	Item* item{};
};

struct SlotDisplayInfo
{
	EquipmentSlot slot{};
	std::string_view label{};
};

// Vertical layout, counted in text rows below the frame's top edge rather than in
// map tiles. A tile is as tall as the zoom makes it and a line of text is not, so
// rows counted in tiles walked off the bottom of the screen: four equipment slots
// and both footer lines were drawn below 896 pixels, and the cursor could still
// reach them.
// Clear air between the last glyph of a right-aligned line and the frame's rule.
inline constexpr int PANEL_EDGE_CLEARANCE = 12;
inline constexpr int DETAIL_BAR_ROWS = 3;
inline constexpr int TAB_ROW = 0;
inline constexpr int FIRST_CONTENT_ROW = 2;
inline constexpr int SLOT_COUNT = 15;

inline constexpr std::array<SlotDisplayInfo, SLOT_COUNT> SLOT_TABLE{ {
	{ EquipmentSlot::HEAD, "Head" },
	{ EquipmentSlot::NECK, "Neck" },
	{ EquipmentSlot::BODY, "Body" },
	{ EquipmentSlot::GIRDLE, "Girdle" },
	{ EquipmentSlot::CLOAK, "Cloak" },
	{ EquipmentSlot::RIGHT_HAND, "Right Hand" },
	{ EquipmentSlot::LEFT_HAND, "Left Hand" },
	{ EquipmentSlot::RIGHT_RING, "Right Ring" },
	{ EquipmentSlot::LEFT_RING, "Left Ring" },
	{ EquipmentSlot::BRACERS, "Bracers" },
	{ EquipmentSlot::GAUNTLETS, "Gauntlets" },
	{ EquipmentSlot::BOOTS, "Boots" },
	{ EquipmentSlot::MISSILE_WEAPON, "Missile" },
	{ EquipmentSlot::MISSILES, "Ammo" },
	{ EquipmentSlot::TOOL, "Tool" },
} };

inline constexpr std::array<ItemCategory, 13> CATEGORY_ORDER{ {
	ItemCategory::CONSUMABLE,
	ItemCategory::SCROLL,
	ItemCategory::WEAPON,
	ItemCategory::ARMOR,
	ItemCategory::HELMET,
	ItemCategory::SHIELD,
	ItemCategory::GAUNTLETS,
	ItemCategory::GIRDLE,
	ItemCategory::JEWELRY,
	ItemCategory::TOOL,
	ItemCategory::TREASURE,
	ItemCategory::QUEST_ITEM,
	ItemCategory::UNKNOWN,
} };

inline constexpr std::array<ItemCategory, 3> USABLE_CATEGORIES{ {
	ItemCategory::CONSUMABLE,
	ItemCategory::SCROLL,
	ItemCategory::UNKNOWN,
} };

class InventoryUI : public BaseMenu
{
public:
	InventoryUI(Player& player, InventoryScreen startScreen, GameContext& ctx);
	~InventoryUI() = default;
	InventoryUI(const InventoryUI&) = delete;
	InventoryUI& operator=(const InventoryUI&) = delete;
	InventoryUI(InventoryUI&&) = delete;
	InventoryUI& operator=(InventoryUI&&) = delete;

	void menu(GameContext& ctx) override;

private:
	// State
	InventoryScreen activeScreen{};
	int equipmentCursor{};
	int listCursor{};
	int scrollOffset{};
	bool filterMode{};
	EquipmentSlot filterSlot{};
	Player& playerRef;

	// Flat item list (shared by Backpack and Usables, rebuilt on tab switch)
	std::vector<BackpackEntry> listEntries;

	// Data building
	void rebuild_item_list(const Player& player, GameContext& ctx);
	// Whether this item may go in this slot. A worn item names its slot in the data and
	// that answer is the only one; the kinds that name none are sorted by what they are.
	bool item_fits_slot(const Item& item, EquipmentSlot slot, const GameContext& ctx) const;
	bool is_usable_category(ItemCategory cat) const;

	// Rendering (all use Renderer via GameContext)
	void render_tab_bar(GameContext& ctx);
	void render_equipment_screen(const Player& player, GameContext& ctx);
	void render_item_list_screen(GameContext& ctx);
	void render_detail_bar(const Player& player, GameContext& ctx);

	// Format helpers
	std::string format_weapon_info(const Item& item) const;
	std::string format_armor_info(const Item& item) const;
	std::string format_stat_bonus_info(const Item& item) const;
	std::string format_enhancement_info(const Item& item) const;
	std::string format_value_info(const Item& item) const;
	std::string get_category_name(ItemCategory cat) const;
	ItemCategory get_effective_category(const Item& item) const;

	// Input handling (uses InputSystem via GameContext)
	bool handle_input(Player& player, GameContext& ctx);
	void handle_cursor_up();
	void handle_cursor_down(GameContext& ctx);
	void handle_tab_switch(int direction);
	void handle_enter_equipment(Player& player, GameContext& ctx);
	void handle_enter_item(Player& player, GameContext& ctx);
	void handle_drop(Player& player, GameContext& ctx);
	// Opens the right-click menu for the row under the pointer, or nothing when that row
	// has no action. Its entries call the same handlers the keyboard does.
	void open_row_menu(Player& player, int mouseRow, GameContext& ctx);
	// Shows a pane and puts its cursor at the top. The one way the active pane changes,
	// so clicking a tab and pressing Tab cannot leave different state behind them.
	void switch_to_screen(InventoryScreen screen);

	// Cursor helpers
	std::optional<int> get_next_item_index(int from, int direction) const;
	Item* get_selected_item() const;

	// Layout: derived from renderer viewport at runtime
	int screen_cols(GameContext& ctx) const;
	int screen_rows(GameContext& ctx) const;

	void draw_frame(GameContext& ctx);
	void draw_highlight_row(int row, GameContext& ctx);
};
