// file: ItemEditor.h
#pragma once

// The item editor: the shared RecordEditor frame over ItemEditorModel, which
// knows items. Entered from the main menu ("Item Editor"); edits the
// ItemRegistry in ctx and saves it to data/content/items.json, with each item's
// tile in the ContentRegistry. The controls are the frame's, in RecordEditor.h.
//
// Usage:
//
//   ItemEditor editor{};     // RecordEditor<ItemEditorModel>
//   editor.enter(ctx);       // needs ctx.itemRegistry and ctx.contentRegistry
//   editor.tick(ctx);        // one frame
//   editor.current_key();    // -> "amulet_of_health", the first item
//   using Field = ItemEditorModel::FieldId;
//   editor.record_model().field_value(Field::VALUE);   // -> "200"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "EditorField.h"
#include "ItemRegistry.h"
#include "RecordEditor.h"
#include "TileRef.h"

struct GameContext;

// What the item editor knows about items: the fields, how each is shown and
// changed, and how items are read from and written to the registries.
class ItemEditorModel
{
private:
	// The selected item as it is being edited, and its tile.
	ItemParams working{};
	TileRef workingTile{};

public:
	enum class FieldId : int
	{
		NAME = 0,
		APPEARANCE,
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

	static constexpr std::size_t FIELD_COUNT =
		static_cast<std::size_t>(FieldId::COUNT);

	// Every field, in FieldId order: how it is edited, its label, and a plain
	// number's member and bounds.
	static constexpr std::array<Descriptor, FIELD_COUNT> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		Descriptor{
			FieldId::APPEARANCE,
			FieldKind::TEXT,
			"Appearance",
			std::nullopt,
		},
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

	// The item editor's colours and words.
	static constexpr EditorLook LOOK{
		.title = "ITEM EDITOR",
		.headerHint =
			"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		.listHint =
			"[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit",
		.fieldsHint =
			"[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/toggle  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit",
		.headerBackground = Color{ 0, 20, 40, 255 },
		.titleText = Color{ 180, 255, 180, 255 },
		.headerHintText = Color{ 100, 130, 100, 255 },
		.listBackground = Color{ 5, 10, 5, 255 },
		.panelDivider = Color{ 80, 120, 80, 255 },
		.listRowFocused = Color{ 0, 60, 0, 200 },
		.listRowSelected = Color{ 0, 40, 0, 150 },
		.listRowHovered = Color{ 20, 30, 20, 160 },
		.listTextSelected = Color{ 150, 255, 150, 255 },
		.listTextPlain = Color{ 200, 200, 200, 255 },
		.fieldsBackground = Color{ 5, 8, 5, 255 },
		.fieldRowFocused = Color{ 0, 60, 0, 200 },
		.fieldRowSelected = Color{ 0, 40, 0, 140 },
		.fieldRowHovered = Color{ 10, 20, 10, 120 },
		.labelSelected = Color{ 150, 255, 150, 255 },
		.labelPlain = Color{ 150, 150, 150, 255 },
		.valueSelected = Color{ 220, 255, 220, 255 },
		.valuePlain = Color{ 200, 200, 200, 255 },
		.tileOutline = Color{ 0, 255, 100, 255 },
		.pickerBackground = Color{ 5, 8, 24, 255 },
		.pickerSubHeader = Color{ 8, 15, 40, 255 },
		.pickerHeaderText = Color{ 120, 200, 200, 255 },
		.pickerCurrent = Color{ 0, 60, 0, 220 },
		.pickerHovered = Color{ 20, 40, 20, 160 },
		.hintBackground = Color{ 0, 20, 40, 255 },
		.savedHint = Color{ 100, 255, 100, 255 },
		.plainHint = Color{ 120, 160, 120, 255 },
	};

	// Every item's key: the shipped ones in key order, then the custom ones.
	[[nodiscard]] std::vector<std::string> keys(const GameContext& ctx) const;
	// Makes the item under key the working copy, with its tile.
	void load(const GameContext& ctx, std::string_view key);
	// Writes the working copy and its tile back under key.
	void commit(GameContext& ctx, std::string_view key) const;
	// Writes items.json and the tile file.
	void save(GameContext& ctx) const;
	// Adds a new item with a weapon's defaults and returns its key.
	[[nodiscard]] std::string add_record(GameContext& ctx) const;
	// Removes a custom item and its tile. The frame refuses a shipped one.
	void remove_record(GameContext& ctx, std::string_view key) const;
	// Whether items.json shipped the item, so it cannot be removed.
	[[nodiscard]] bool is_builtin(
		const GameContext& ctx,
		std::string_view key) const;

	// The list's text for an item that is not selected: its name, or its key
	// made readable when it has none.
	[[nodiscard]] std::string row_label(
		const GameContext& ctx,
		std::string_view key) const;
	// The list's text for the selected item, from the working copy.
	[[nodiscard]] std::string working_row_label(std::string_view key) const;
	// The tile the list shows for an item that is not selected.
	[[nodiscard]] TileRef row_tile(
		const GameContext& ctx,
		std::string_view key) const;

	[[nodiscard]] TileRef working_tile() const { return workingTile; }

	void set_working_tile(TileRef tile) { workingTile = tile; }

	[[nodiscard]] std::string field_label(FieldId field) const;
	[[nodiscard]] std::string field_value(FieldId field) const;
	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);
};

// The item editor itself; its members are defined in RecordEditor.cpp.
using ItemEditor = RecordEditor<ItemEditorModel>;
extern template class RecordEditor<ItemEditorModel>;
