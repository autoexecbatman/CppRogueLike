// file: ItemEditor.h
#pragma once

// Full-screen in-game item data editor.
// Entered from main menu ("Item Editor" option).
// Edits the ItemRegistry in ctx and saves it to data/content/items.json.
//
// Controls:
//   Tab              -- switch focus: list panel <-> field panel
//   Up / Down        -- navigate item list or field list
//   Left / Right     -- adjust integer field value (+/-1)
//   Ctrl+Left/Right  -- adjust integer field value (+/-10)
//   Enter            -- edit string field / toggle enum / open tile picker
//   F2               -- open tile picker for current item tile
//   Ctrl+S           -- save items.json
//   Esc              -- commit edits and exit to main menu

#include <string>
#include <vector>

#include "ItemRegistry.h"
#include "Renderer.h"

struct GameContext;
class ContentRegistry;

class ItemEditor
{
private:
	enum class Mode
	{
		NORMAL,
		EDIT_STRING,
		TILE_PICKER
	};

	enum class FieldId : int
	{
		NAME = 0,
		CATEGORY,
		ITEM_CLASS,
		PICKABLE_TYPE,
		COLOR,
		VALUE,
		BASE_WEIGHT,
		WEIGHT,
		LEVEL_MIN,
		LEVEL_MAX,
		LEVEL_SCALING,
		CONSUMABLE_EFFECT,
		CONSUMABLE_BUFF,
		CONSUMABLE_AMT,
		DURATION,
		TARGET_MODE,
		SCROLL_ANIM,
		RANGE,
		DAMAGE,
		CONFUSE_TURNS,
		RANGED,
		HAND_REQUIREMENT,
		WEAPON_SIZE,
		STRENGTH_RATING,
		AC_BONUS,
		EFFECT,
		EFFECT_BONUS,
		STR_BONUS,
		DEX_BONUS,
		CON_BONUS,
		INT_BONUS,
		WIS_BONUS,
		CHA_BONUS,
		IS_SET_MODE,
		EXCEPTIONAL_STRENGTH,
		NUTRITION,
		TILE,
		COUNT
	};

	static constexpr int FIELD_COUNT = static_cast<int>(FieldId::COUNT);

	ContentRegistry* contentRegistry{ nullptr };
	bool active{ false };
	Mode mode{ Mode::NORMAL };
	int focus{ 0 }; // 0 = list panel, 1 = field panel

	// Item list (all registered keys)
	std::vector<std::string> keys;
	int listCursor{ 0 };
	int listScroll{ 0 };

	// Working copy of selected item
	ItemParams working{};
	TileRef workingTile{};

	// Field navigation
	int fieldCursor{ 0 };
	int fieldScroll{ 0 };
	std::string editBuffer;

	// Tile picker state
	int pickerSheet{ 0 };
	int pickerScroll{ 0 };

	// Status feedback
	double lastSaveTime{ -100.0 };

	void load_working(const ItemRegistry& items);
	void commit_working(ItemRegistry& items);

	void handle_input(GameContext& ctx);
	void handle_normal(const GameContext& ctx);
	void handle_edit_string();
	void handle_picker(const Renderer& r);

	void render(const GameContext& ctx) const;
	void render_header(const Renderer& r) const;
	void render_list(const Renderer& r, const ItemRegistry& items) const;
	void render_fields(const Renderer& r) const;
	void render_picker(const Renderer& r) const;
	void render_hint(const Renderer& r) const;

	[[nodiscard]] const std::string& current_key() const;
	[[nodiscard]] FieldId current_field() const;

	[[nodiscard]] std::string field_label(FieldId f) const;
	[[nodiscard]] std::string field_value(FieldId f) const;
	[[nodiscard]] bool field_is_string(FieldId f) const;
	[[nodiscard]] bool field_is_toggle(FieldId f) const;

	void field_adjust(FieldId f, int delta);
	void field_toggle(FieldId f);
	void field_set_string(FieldId f, std::string val);

public:
	void enter(GameContext& ctx);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
