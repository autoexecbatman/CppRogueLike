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

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "EditorField.h"
#include "ItemRegistry.h"
#include "ListCursor.h"
#include "TileRef.h"

struct GameContext;
class ContentRegistry;
class Renderer;

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
		CONSUMABLE_AMT_DICE,
		CONSUMABLE_AMT_SIDES,
		CONSUMABLE_AMT_BONUS,
		DURATION_DICE,
		DURATION_SIDES,
		DURATION_BONUS,
		DURATION_UNIT,
		AGES_YEARS,
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
		APPLICATION,
		EXCEPTIONAL_STRENGTH,
		NUTRITION,
		TILE,
		COUNT
	};

	// A plain number's member and bounds, read from the working ItemParams.
	using Plain = PlainNumber<ItemParams>;
	// One field's entry in FIELDS.
	using Descriptor = FieldDescriptor<FieldId, ItemParams>;

	static constexpr size_t FIELD_COUNT = static_cast<size_t>(FieldId::COUNT);

	ContentRegistry* contentRegistry{ nullptr };
	bool active{ false };
	Mode mode{ Mode::NORMAL };
	int focus{ 0 }; // 0 = list panel, 1 = field panel

	// Item list (all registered keys)
	std::vector<std::string> keys;
	ListCursor listCursor{};

	// Working copy of selected item
	ItemParams working{};
	TileRef workingTile{};

	// Field navigation
	ListCursor fieldCursor{};
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
	void handle_picker(const Renderer& renderer);

	void render(const GameContext& ctx) const;
	void render_header(const Renderer& renderer) const;
	// The height of the panels between the header and the hint bar. Input and drawing both
	// read it, so a click and the row drawn under it come from one row count.
	//
	// Example:
	//   body_height(renderer);   // a 900-pixel screen -> 824, after a 48 header and a 28 hint bar
	[[nodiscard]] static int body_height(const Renderer& renderer);
	void render_list(const Renderer& renderer, const ItemRegistry& items) const;
	void render_fields(const Renderer& renderer) const;
	void render_picker(const Renderer& renderer) const;
	void render_hint(const Renderer& renderer) const;

	[[nodiscard]] const std::string& current_key() const;
	[[nodiscard]] FieldId current_field() const;

	[[nodiscard]] std::string field_label(FieldId field) const;
	[[nodiscard]] std::string field_value(FieldId field) const;

	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);

public:
	// Every field, in FieldId order: how it is edited, its label, and a plain
	// number's member and bounds.
	static constexpr std::array<Descriptor, FIELD_COUNT> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		Descriptor{
			FieldId::CATEGORY,
			FieldKind::TEXT,
			"Category",
			std::nullopt,
		},
		Descriptor{
			FieldId::ITEM_CLASS,
			FieldKind::TOGGLE,
			"Item Class",
			std::nullopt,
		},
		Descriptor{
			FieldId::PICKABLE_TYPE,
			FieldKind::TOGGLE,
			"Pickable Type",
			std::nullopt,
		},
		Descriptor{ FieldId::COLOR, FieldKind::TOGGLE, "Color", std::nullopt },
		Descriptor{
			FieldId::VALUE,
			FieldKind::ADJUST,
			"Value",
			Plain{ &ItemParams::value, 0, 99999 },
		},
		Descriptor{
			FieldId::BASE_WEIGHT,
			FieldKind::ADJUST,
			"Spawn Weight",
			Plain{ &ItemParams::baseWeight, 0, 100 },
		},
		Descriptor{
			FieldId::WEIGHT,
			FieldKind::ADJUST,
			"Carry Weight",
			Plain{ &ItemParams::weight, 0, 1000 },
		},
		Descriptor{
			FieldId::LEVEL_MIN,
			FieldKind::ADJUST,
			"Level Min",
			Plain{ &ItemParams::levelMin, 1, 20 },
		},
		Descriptor{
			FieldId::LEVEL_MAX,
			FieldKind::ADJUST,
			"Level Max",
			Plain{ &ItemParams::levelMax, 0, 20 },
		},
		Descriptor{
			FieldId::LEVEL_SCALING,
			FieldKind::ADJUST,
			"Level Scaling",
			std::nullopt,
		},
		Descriptor{
			FieldId::CONSUMABLE_EFFECT,
			FieldKind::TOGGLE,
			"Consumable Effect",
			std::nullopt,
		},
		Descriptor{
			FieldId::CONSUMABLE_BUFF,
			FieldKind::TOGGLE,
			"Consumable Buff",
			std::nullopt,
		},
		Descriptor{
			FieldId::CONSUMABLE_AMT_DICE,
			FieldKind::ADJUST,
			"Amount Dice",
			std::nullopt,
		},
		Descriptor{
			FieldId::CONSUMABLE_AMT_SIDES,
			FieldKind::ADJUST,
			"Amount Sides",
			std::nullopt,
		},
		Descriptor{
			FieldId::CONSUMABLE_AMT_BONUS,
			FieldKind::ADJUST,
			"Amount Bonus",
			std::nullopt,
		},
		Descriptor{
			FieldId::DURATION_DICE,
			FieldKind::ADJUST,
			"Duration Dice",
			std::nullopt,
		},
		Descriptor{
			FieldId::DURATION_SIDES,
			FieldKind::ADJUST,
			"Duration Sides",
			std::nullopt,
		},
		Descriptor{
			FieldId::DURATION_BONUS,
			FieldKind::ADJUST,
			"Duration Bonus",
			std::nullopt,
		},
		Descriptor{
			FieldId::DURATION_UNIT,
			FieldKind::TOGGLE,
			"Duration Unit",
			std::nullopt,
		},
		Descriptor{
			FieldId::AGES_YEARS,
			FieldKind::ADJUST,
			"Ages Drinker (years)",
			Plain{ &ItemParams::agesYears, -99, 99 },
		},
		Descriptor{
			FieldId::TARGET_MODE,
			FieldKind::TOGGLE,
			"Target Mode",
			std::nullopt,
		},
		Descriptor{
			FieldId::SCROLL_ANIM,
			FieldKind::TOGGLE,
			"Scroll Animation",
			std::nullopt,
		},
		Descriptor{
			FieldId::RANGE,
			FieldKind::ADJUST,
			"Range",
			Plain{ &ItemParams::range, 0, 50 },
		},
		Descriptor{
			FieldId::DAMAGE,
			FieldKind::ADJUST,
			"Damage",
			Plain{ &ItemParams::damage, 0, 999 },
		},
		Descriptor{
			FieldId::CONFUSE_TURNS,
			FieldKind::ADJUST,
			"Confuse Turns",
			Plain{ &ItemParams::confuseTurns, 0, 999 },
		},
		Descriptor{
			FieldId::RANGED,
			FieldKind::TOGGLE,
			"Ranged",
			std::nullopt,
		},
		Descriptor{
			FieldId::HAND_REQUIREMENT,
			FieldKind::TOGGLE,
			"Hand Requirement",
			std::nullopt,
		},
		Descriptor{
			FieldId::WEAPON_SIZE,
			FieldKind::TOGGLE,
			"Weapon Size",
			std::nullopt,
		},
		// 0 is an ordinary weapon; a rating is a Strength score, and Table 1
		// stops at 25.
		Descriptor{
			FieldId::STRENGTH_RATING,
			FieldKind::ADJUST,
			"Made For STR",
			Plain{ &ItemParams::strengthRating, 0, 25 },
		},
		Descriptor{
			FieldId::AC_BONUS,
			FieldKind::ADJUST,
			"AC Bonus",
			Plain{ &ItemParams::acBonus, -10, 10 },
		},
		Descriptor{
			FieldId::EFFECT,
			FieldKind::TOGGLE,
			"Effect",
			std::nullopt,
		},
		Descriptor{
			FieldId::EFFECT_BONUS,
			FieldKind::ADJUST,
			"Effect Bonus",
			Plain{ &ItemParams::effectBonus, 0, 20 },
		},
		Descriptor{
			FieldId::STR_BONUS,
			FieldKind::ADJUST,
			"STR Bonus",
			Plain{ &ItemParams::strBonus, -18, 18 },
		},
		Descriptor{
			FieldId::DEX_BONUS,
			FieldKind::ADJUST,
			"DEX Bonus",
			Plain{ &ItemParams::dexBonus, -18, 18 },
		},
		Descriptor{
			FieldId::CON_BONUS,
			FieldKind::ADJUST,
			"CON Bonus",
			Plain{ &ItemParams::conBonus, -18, 18 },
		},
		Descriptor{
			FieldId::INT_BONUS,
			FieldKind::ADJUST,
			"INT Bonus",
			Plain{ &ItemParams::intBonus, -18, 18 },
		},
		Descriptor{
			FieldId::WIS_BONUS,
			FieldKind::ADJUST,
			"WIS Bonus",
			Plain{ &ItemParams::wisBonus, -18, 18 },
		},
		Descriptor{
			FieldId::CHA_BONUS,
			FieldKind::ADJUST,
			"CHA Bonus",
			Plain{ &ItemParams::chaBonus, -18, 18 },
		},
		Descriptor{
			FieldId::APPLICATION,
			FieldKind::TOGGLE,
			"Application",
			std::nullopt,
		},
		Descriptor{
			FieldId::EXCEPTIONAL_STRENGTH,
			FieldKind::ADJUST,
			"Exceptional STR",
			Plain{ &ItemParams::exceptionalStrength, 0, 100 },
		},
		Descriptor{
			FieldId::NUTRITION,
			FieldKind::ADJUST,
			"Nutrition Value",
			Plain{ &ItemParams::nutritionValue, 0, 9999 },
		},
		Descriptor{ FieldId::TILE, FieldKind::TILE, "Tile", std::nullopt },
	} };

	void enter(GameContext& ctx);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
