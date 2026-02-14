#pragma once

#include <vector>
#include <string>
#include <array>

#include "../Actor/EquipmentSlot.h"
#include "../Items/ItemClassification.h"

class Creature;
class Item;
class Player;
struct GameContext;

enum class InventoryScreen
{
	EQUIPMENT,
	BACKPACK,
	USABLES
};

struct BackpackEntry
{
	enum class Kind
	{
		CATEGORY_HEADER,
		ITEM
	};

	Kind kind;
	ItemCategory category;
	std::string header_text;
	Item* item;
};

struct SlotDisplayInfo
{
	EquipmentSlot slot;
	const char* label;
};

inline constexpr int DETAIL_BAR_HEIGHT = 4;
inline constexpr int TAB_BAR_HEIGHT = 1;
inline constexpr int SLOT_COUNT = 15;

inline constexpr std::array<SlotDisplayInfo, SLOT_COUNT> SLOT_TABLE
{{
	{EquipmentSlot::HEAD,           "Head"},
	{EquipmentSlot::NECK,           "Neck"},
	{EquipmentSlot::BODY,           "Body"},
	{EquipmentSlot::GIRDLE,         "Girdle"},
	{EquipmentSlot::CLOAK,          "Cloak"},
	{EquipmentSlot::RIGHT_HAND,     "Right Hand"},
	{EquipmentSlot::LEFT_HAND,      "Left Hand"},
	{EquipmentSlot::RIGHT_RING,     "Right Ring"},
	{EquipmentSlot::LEFT_RING,      "Left Ring"},
	{EquipmentSlot::BRACERS,        "Bracers"},
	{EquipmentSlot::GAUNTLETS,      "Gauntlets"},
	{EquipmentSlot::BOOTS,          "Boots"},
	{EquipmentSlot::MISSILE_WEAPON, "Missile"},
	{EquipmentSlot::MISSILES,       "Ammo"},
	{EquipmentSlot::TOOL,           "Tool"},
}};

inline constexpr std::array<ItemCategory, 13> CATEGORY_ORDER
{{
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
}};

inline constexpr std::array<ItemCategory, 3> USABLE_CATEGORIES
{{
	ItemCategory::CONSUMABLE,
	ItemCategory::SCROLL,
	ItemCategory::UNKNOWN,
}};

class InventoryUI
{
public:
	InventoryUI();
	~InventoryUI() = default;

	void display(Player& player, GameContext& ctx, InventoryScreen startScreen);

private:
	// Data building
	void rebuild_item_list(const Player& player, GameContext& ctx);
	bool item_fits_slot(const Item& item, EquipmentSlot slot) const;
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
	void handle_tab_switch();
	void handle_enter_equipment(Player& player, GameContext& ctx);
	void handle_enter_item(Player& player, GameContext& ctx);
	void handle_drop(Player& player, GameContext& ctx);

	// Cursor helpers
	int get_next_item_index(int from, int direction) const;
	Item* get_selected_item() const;

	// Layout: derived from renderer viewport at runtime
	int screen_cols(GameContext& ctx) const;
	int screen_rows(GameContext& ctx) const;

	// State
	InventoryScreen activeScreen;
	int equipmentCursor;
	int listCursor;
	int scrollOffset;
	bool filterMode;
	EquipmentSlot filterSlot;

	// Flat item list (shared by Backpack and Usables, rebuilt on tab switch)
	std::vector<BackpackEntry> listEntries;
};
