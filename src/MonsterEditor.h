// file: MonsterEditor.h
#pragma once

// The monster editor: the shared RecordEditor frame over MonsterEditorModel,
// which knows monsters. Entered from the main menu ("Monster Editor"); edits
// the MonsterRegistry in ctx and saves it to data/content/monsters.json. The
// controls are the frame's, in RecordEditor.h.
//
// Usage:
//
//   MonsterEditor editor{};   // RecordEditor<MonsterEditorModel>
//   editor.enter(ctx);        // needs ctx.monsterRegistry
//   editor.tick(ctx);         // one frame
//   editor.current_key();     // -> "goblin", the first standard monster
//   using Field = MonsterEditorModel::FieldId;
//   editor.record_model().field_value(Field::THACO);   // -> "20"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "EditorField.h"
#include "MonsterRegistry.h"
#include "RecordEditor.h"
#include "TileRef.h"

struct GameContext;

// What the monster editor knows about monsters: the fields, how each is shown
// and changed, and how monsters are read from and written to the registry.
class MonsterEditorModel
{
private:
	// The selected monster as it is being edited; its tile is its symbol.
	MonsterParams working{};

	// Which of the working monster's hit dice counts the XP field edits.
	size_t selectedXpCount{ 0 };

	// Which of the working monster's innate powers the spell and frequency
	// fields edit.
	size_t selectedPower{ 0 };

	// Rebuilds the working damage from dice, so its text and its range follow.
	void set_damage_dice(int num, int sides, int bonus);

public:
	enum class FieldId
	{
		NAME,
		CORPSE,
		CORPSE_NUTRITION,
		CORPSE_TASTE,
		CORPSE_EDIBLE,
		HD_LOWEST,
		HD_HIGHEST,
		HD_SIDES,
		HD_BONUS,
		THACO,
		AC,
		XP_COUNT,
		XP,
		DR,
		STR_SOURCE,
		STR_SCORE,
		STR_EXCEPTIONAL,
		WEAPON,
		DMG_NUM,
		DMG_SIDES,
		DMG_BONUS,
		DMG_TYPE,
		AI_TYPE,
		ETHICS,
		MORALITY,
		SIZE,
		INTELLIGENCE,
		CAN_SWIM,
		CAN_FLY,
		WALKS_WEBS,
		ATTITUDE,
		DISPLACEABLE,
		CASTER_CLASS,
		CASTER_LEVEL,
		INNATE_POWER,
		INNATE_SPELL,
		INNATE_FREQUENCY,
		WEIGHT,
		DEPTH_MIN,
		DEPTH_MAX,
		TILE,
		COUNT
	};

	// A plain number's member and bounds, read from the working MonsterParams.
	using Plain = PlainNumber<MonsterParams>;
	// One field's entry in FIELDS.
	using Descriptor = FieldDescriptor<FieldId, MonsterParams>;

	static constexpr std::size_t FIELD_COUNT =
		static_cast<std::size_t>(FieldId::COUNT);

	// Every field, in FieldId order: how it is edited, its label, and a plain
	// number's member and bounds.
	static constexpr std::array<Descriptor, FIELD_COUNT> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		Descriptor{ FieldId::CORPSE, FieldKind::TEXT, "Corpse", std::nullopt },
		Descriptor{
			FieldId::CORPSE_NUTRITION,
			FieldKind::ADJUST,
			"Corpse Nutrition",
			Plain{ &MonsterParams::corpseNutrition, 0, 9999 },
		},
		Descriptor{
			FieldId::CORPSE_TASTE,
			FieldKind::TEXT,
			"Corpse Taste",
			std::nullopt,
		},
		Descriptor{
			FieldId::CORPSE_EDIBLE,
			FieldKind::TOGGLE,
			"Corpse Edible",
			std::nullopt,
		},
		Descriptor{
			FieldId::HD_LOWEST,
			FieldKind::ADJUST,
			"HD Lowest",
			std::nullopt,
		},
		Descriptor{
			FieldId::HD_HIGHEST,
			FieldKind::ADJUST,
			"HD Highest",
			std::nullopt,
		},
		Descriptor{
			FieldId::HD_SIDES,
			FieldKind::ADJUST,
			"HD Sides",
			std::nullopt,
		},
		Descriptor{
			FieldId::HD_BONUS,
			FieldKind::ADJUST,
			"HD Bonus",
			std::nullopt,
		},
		Descriptor{
			FieldId::THACO,
			FieldKind::ADJUST,
			"THAC0",
			Plain{ &MonsterParams::thaco, 1, 25 },
		},
		Descriptor{
			FieldId::AC,
			FieldKind::ADJUST,
			"AC",
			Plain{ &MonsterParams::ac, -10, 10 },
		},
		Descriptor{
			FieldId::XP_COUNT,
			FieldKind::ADJUST,
			"XP For HD",
			std::nullopt,
		},
		Descriptor{ FieldId::XP, FieldKind::ADJUST, "XP", std::nullopt },
		Descriptor{
			FieldId::DR,
			FieldKind::ADJUST,
			"DR",
			Plain{ &MonsterParams::dr, 0, 20 },
		},
		Descriptor{
			FieldId::STR_SOURCE,
			FieldKind::TOGGLE,
			"Strength",
			std::nullopt,
		},
		Descriptor{
			FieldId::STR_SCORE,
			FieldKind::ADJUST,
			"STR Score",
			std::nullopt,
		},
		Descriptor{
			FieldId::STR_EXCEPTIONAL,
			FieldKind::ADJUST,
			"STR Exceptional",
			std::nullopt,
		},
		Descriptor{
			FieldId::WEAPON,
			FieldKind::TEXT,
			"Natural Attack",
			std::nullopt,
		},
		Descriptor{
			FieldId::DMG_NUM,
			FieldKind::ADJUST,
			"Dmg Num",
			std::nullopt,
		},
		Descriptor{
			FieldId::DMG_SIDES,
			FieldKind::ADJUST,
			"Dmg Sides",
			std::nullopt,
		},
		Descriptor{
			FieldId::DMG_BONUS,
			FieldKind::ADJUST,
			"Dmg Bonus",
			std::nullopt,
		},
		Descriptor{
			FieldId::DMG_TYPE,
			FieldKind::TOGGLE,
			"Dmg Type",
			std::nullopt,
		},
		Descriptor{
			FieldId::AI_TYPE,
			FieldKind::TOGGLE,
			"AI Type",
			std::nullopt,
		},
		Descriptor{
			FieldId::ETHICS,
			FieldKind::TOGGLE,
			"Ethics",
			std::nullopt,
		},
		Descriptor{
			FieldId::MORALITY,
			FieldKind::TOGGLE,
			"Morality",
			std::nullopt,
		},
		Descriptor{ FieldId::SIZE, FieldKind::TOGGLE, "Size", std::nullopt },
		Descriptor{
			FieldId::INTELLIGENCE,
			FieldKind::TOGGLE,
			"Intelligence",
			std::nullopt,
		},
		Descriptor{
			FieldId::CAN_SWIM,
			FieldKind::TOGGLE,
			"Can Swim",
			std::nullopt,
		},
		Descriptor{
			FieldId::CAN_FLY,
			FieldKind::TOGGLE,
			"Can Fly",
			std::nullopt,
		},
		Descriptor{
			FieldId::WALKS_WEBS,
			FieldKind::TOGGLE,
			"Walks Webs",
			std::nullopt,
		},
		Descriptor{
			FieldId::ATTITUDE,
			FieldKind::TOGGLE,
			"Attitude",
			std::nullopt,
		},
		Descriptor{
			FieldId::DISPLACEABLE,
			FieldKind::TOGGLE,
			"Displaceable",
			std::nullopt,
		},
		Descriptor{
			FieldId::CASTER_CLASS,
			FieldKind::TOGGLE,
			"Caster Class",
			std::nullopt,
		},
		Descriptor{
			FieldId::CASTER_LEVEL,
			FieldKind::ADJUST,
			"Caster Level",
			std::nullopt,
		},
		Descriptor{
			FieldId::INNATE_POWER,
			FieldKind::ADJUST,
			"Innate Power",
			std::nullopt,
		},
		Descriptor{
			FieldId::INNATE_SPELL,
			FieldKind::TEXT,
			"Power Spell",
			std::nullopt,
		},
		Descriptor{
			FieldId::INNATE_FREQUENCY,
			FieldKind::TOGGLE,
			"Power Frequency",
			std::nullopt,
		},
		Descriptor{
			FieldId::WEIGHT,
			FieldKind::ADJUST,
			"Spawn Weight",
			Plain{ &MonsterParams::baseWeight, 0, 100 },
		},
		Descriptor{
			FieldId::DEPTH_MIN,
			FieldKind::ADJUST,
			"Depth Min",
			Plain{ &MonsterParams::levelMinimum, 1, 20 },
		},
		Descriptor{
			FieldId::DEPTH_MAX,
			FieldKind::ADJUST,
			"Depth Max",
			Plain{ &MonsterParams::levelMaximum, 0, 20 },
		},
		Descriptor{ FieldId::TILE, FieldKind::TILE, "Tile", std::nullopt },
	} };

	// The monster editor's colours and words.
	static constexpr EditorLook LOOK{
		.title = "MONSTER EDITOR",
		.headerHint =
			"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		.listHint =
			"[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit",
		.fieldsHint =
			"[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit",
		.headerBackground = Color{ 20, 20, 40, 255 },
		.titleText = Color{ 255, 255, 180, 255 },
		.headerHintText = Color{ 130, 130, 100, 255 },
		.listBackground = Color{ 10, 10, 20, 255 },
		.panelDivider = Color{ 80, 80, 120, 255 },
		.listRowFocused = Color{ 60, 60, 0, 200 },
		.listRowSelected = Color{ 40, 40, 0, 150 },
		.listRowHovered = Color{ 30, 30, 30, 160 },
		.listTextSelected = Color{ 255, 255, 100, 255 },
		.listTextPlain = Color{ 200, 200, 200, 255 },
		.fieldsBackground = Color{ 8, 8, 16, 255 },
		.fieldRowFocused = Color{ 60, 60, 0, 200 },
		.fieldRowSelected = Color{ 40, 40, 0, 140 },
		.fieldRowHovered = Color{ 20, 20, 10, 120 },
		.labelSelected = Color{ 255, 220, 80, 255 },
		.labelPlain = Color{ 150, 150, 150, 255 },
		.valueSelected = Color{ 255, 255, 200, 255 },
		.valuePlain = Color{ 200, 200, 200, 255 },
		.tileOutline = Color{ 255, 255, 0, 255 },
		.pickerBackground = Color{ 8, 8, 24, 255 },
		.pickerSubHeader = Color{ 15, 15, 40, 255 },
		.pickerHeaderText = Color{ 200, 200, 120, 255 },
		.pickerCurrent = Color{ 60, 60, 0, 220 },
		.pickerHovered = Color{ 40, 40, 20, 160 },
		.hintBackground = Color{ 20, 20, 40, 255 },
		.savedHint = Color{ 100, 255, 100, 255 },
		.plainHint = Color{ 160, 160, 120, 255 },
	};

	// Every monster's key: the standard ones in their fixed order, then the
	// custom ones in key order.
	[[nodiscard]] std::vector<std::string> keys(const GameContext& ctx) const;
	// Makes the monster under key the working copy, with its first hit dice
	// count and first innate power selected.
	void load(const GameContext& ctx, std::string_view key);
	// Writes the working copy, tile included, back under key.
	void commit(GameContext& ctx, std::string_view key) const;
	// Writes monsters.json.
	void save(GameContext& ctx) const;
	// Adds a new monster with a weak melee creature's defaults and returns its
	// key.
	[[nodiscard]] std::string add_record(GameContext& ctx) const;
	// Removes a custom monster. The frame refuses a standard one.
	void remove_record(GameContext& ctx, std::string_view key) const;
	// Whether the monster is a standard one, so it cannot be removed.
	[[nodiscard]] bool is_builtin(
		const GameContext& ctx,
		std::string_view key) const;

	// The list's text for a monster that is not selected: its name, or its key
	// made readable when it has none.
	[[nodiscard]] std::string row_label(
		const GameContext& ctx,
		std::string_view key) const;
	// The list's text for the selected monster, from the working copy.
	[[nodiscard]] std::string working_row_label(std::string_view key) const;
	// The tile the list shows for a monster that is not selected.
	[[nodiscard]] TileRef row_tile(
		const GameContext& ctx,
		std::string_view key) const;

	[[nodiscard]] TileRef working_tile() const { return working.symbol; }

	void set_working_tile(TileRef tile) { working.symbol = tile; }

	[[nodiscard]] std::string field_label(FieldId field) const;
	[[nodiscard]] std::string field_value(FieldId field) const;
	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);
};

// The monster editor itself; its members are defined in RecordEditor.cpp.
using MonsterEditor = RecordEditor<MonsterEditorModel>;
extern template class RecordEditor<MonsterEditorModel>;
