// file: SpellEditor.h
#pragma once

// The spell editor: the shared RecordEditor frame over SpellEditorModel, which
// knows spells. Entered from the main menu ("Spell Editor"); edits the
// SpellRegistry in ctx and saves it to data/content/spells.json. Spells have
// no tiles, so the editor has no tile picker. The controls are the frame's, in
// RecordEditor.h.
//
// Usage:
//
//   SpellEditor editor{};   // RecordEditor<SpellEditorModel>
//   editor.enter(ctx);      // needs ctx.spellRegistry
//   editor.tick(ctx);       // one frame
//   editor.current_key();   // -> "cure_light_wounds", the first builtin
//   using Field = SpellEditorModel::FieldId;
//   editor.record_model().field_value(Field::LEVEL);   // -> "1"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "EditorField.h"
#include "RecordEditor.h"
#include "SpellRegistry.h"

struct GameContext;

// What the spell editor knows about spells: the fields, how each is shown and
// changed, and how spells are read from and written to the registry.
class SpellEditorModel
{
private:
	// The selected spell as it is being edited.
	SpellDefinition working{};

public:
	enum class FieldId
	{
		NAME,
		LEVEL,
		CLASS,
		EFFECT_TYPE,
		DESCRIPTION,
		COUNT
	};

	// A plain number's member and bounds, read from the working spell.
	using Plain = PlainNumber<SpellDefinition>;
	// One field's entry in FIELDS.
	using Descriptor = FieldDescriptor<FieldId, SpellDefinition>;

	static constexpr std::size_t FIELD_COUNT =
		static_cast<std::size_t>(FieldId::COUNT);

	// Every field, in FieldId order: how it is edited, its label, and a plain
	// number's member and bounds.
	static constexpr std::array<Descriptor, FIELD_COUNT> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		// The spell levels the rules print run from 1st to 9th.
		Descriptor{
			FieldId::LEVEL,
			FieldKind::ADJUST,
			"Level",
			Plain{ &SpellDefinition::level, 1, 9 },
		},
		Descriptor{ FieldId::CLASS, FieldKind::TOGGLE, "Class", std::nullopt },
		Descriptor{
			FieldId::EFFECT_TYPE,
			FieldKind::CYCLE,
			"Effect",
			std::nullopt,
		},
		Descriptor{
			FieldId::DESCRIPTION,
			FieldKind::TEXT,
			"Description",
			std::nullopt,
		},
	} };

	// The spell editor's colours and words. It has no tile picker, so it gives
	// no tile or picker colours.
	static constexpr EditorLook LOOK{
		.title = "SPELL EDITOR",
		.headerHint =
			"Tab:switch focus  Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Ctrl+S:save  Esc:exit",
		.listHint =
			"[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit",
		.fieldsHint =
			"[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Tab:switch  Ctrl+S:save  Esc:exit",
		.headerBackground = Color{ 20, 20, 40, 255 },
		.titleText = Color{ 180, 255, 180, 255 },
		.headerHintText = Color{ 130, 130, 100, 255 },
		.listBackground = Color{ 10, 10, 20, 255 },
		.panelDivider = Color{ 80, 80, 120, 255 },
		.listRowFocused = Color{ 0, 60, 0, 200 },
		.listRowSelected = Color{ 0, 40, 0, 150 },
		.listRowHovered = Color{ 30, 30, 30, 160 },
		.listTextSelected = Color{ 180, 255, 180, 255 },
		.listTextPlain = Color{ 200, 200, 200, 255 },
		.fieldsBackground = Color{ 8, 8, 16, 255 },
		.fieldRowFocused = Color{ 0, 60, 0, 200 },
		.fieldRowSelected = Color{ 0, 40, 0, 140 },
		.fieldRowHovered = Color{ 20, 30, 20, 120 },
		.labelSelected = Color{ 180, 255, 180, 255 },
		.labelPlain = Color{ 150, 150, 150, 255 },
		.valueSelected = Color{ 255, 255, 200, 255 },
		.valuePlain = Color{ 200, 200, 200, 255 },
		.hintBackground = Color{ 20, 20, 40, 255 },
		.savedHint = Color{ 100, 255, 100, 255 },
		.plainHint = Color{ 160, 160, 120, 255 },
	};

	// Every spell's key: the builtin ones in their fixed order, then the
	// custom ones in key order.
	[[nodiscard]] std::vector<std::string> keys(const GameContext& ctx) const;
	// Makes the spell under key the working copy.
	void load(const GameContext& ctx, std::string_view key);
	// Writes the working copy back under key.
	void commit(GameContext& ctx, std::string_view key) const;
	// Writes spells.json.
	void save(GameContext& ctx) const;
	// Adds a new 1st-level wizard spell with no effect and returns its key.
	[[nodiscard]] std::string add_record(GameContext& ctx) const;
	// Removes a custom spell. The frame refuses a builtin one.
	void remove_record(GameContext& ctx, std::string_view key) const;
	// Whether the spell is a builtin one, so it cannot be removed.
	[[nodiscard]] bool is_builtin(
		const GameContext& ctx,
		std::string_view key) const;

	// The list's text for a spell that is not selected: its level and name.
	[[nodiscard]] std::string row_label(
		const GameContext& ctx,
		std::string_view key) const;
	// The list's text for the selected spell, from the working copy.
	[[nodiscard]] std::string working_row_label(std::string_view key) const;
	// The class a spell that is not selected is marked with: "C" for a cleric
	// spell, "W" otherwise.
	[[nodiscard]] std::optional<RowTag> row_tag(
		const GameContext& ctx,
		std::string_view key) const;
	// The selected spell's class mark, from the working copy.
	[[nodiscard]] std::optional<RowTag> working_row_tag() const;

	[[nodiscard]] std::string field_label(FieldId field) const;
	[[nodiscard]] std::string field_value(FieldId field) const;
	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);
};

// The spell editor itself; its members are defined in RecordEditor.cpp.
using SpellEditor = RecordEditor<SpellEditorModel>;
extern template class RecordEditor<SpellEditorModel>;
