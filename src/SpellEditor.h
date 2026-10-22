// file: SpellEditor.h
#pragma once

// Full-screen in-game spell metadata editor.
// Entered from main menu ("Spell Editor" option).
// Edits the SpellRegistry in ctx and saves it to data/content/spells.json.
//
// Controls:
//   Tab          -- switch focus: list <-> fields
//   Up / Down    -- navigate spell list or field list
//   Left / Right -- adjust Level field (+/-1)
//   Enter        -- edit string field / cycle class field
//   Ctrl+S       -- save spells.json
//   Esc          -- commit edits and exit to main menu

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "EditorField.h"
#include "ListCursor.h"
#include "SpellRegistry.h"

struct GameContext;
class Renderer;

enum class Mode
{
	NORMAL,
	EDIT_STRING
};

enum class FieldId
{
	NAME,
	LEVEL,
	CLASS,
	EFFECT_TYPE,
	DESCRIPTION,
	COUNT
};

class SpellEditor
{
private:
	// One field's entry in FIELDS.
	using Descriptor = FieldDescriptor<FieldId, SpellDefinition>;

	bool active{ false };
	Mode mode{ Mode::NORMAL };
	int focus{ 0 }; // 0 = list, 1 = fields

	// Spell list (string keys: builtins then custom)
	std::vector<std::string> keys;
	ListCursor listCursor{};

	// Working copy
	SpellDefinition working{};

	// Field navigation
	ListCursor fieldCursor{};
	std::string editBuffer;

	// Status feedback
	double lastSaveTime{ -100.0 };

	void load_working(const SpellRegistry& spells);
	void commit_working(SpellRegistry& spells);

	void handle_input(GameContext& ctx);
	void handle_normal(const GameContext& ctx);
	void handle_edit_string();

	void render(const GameContext& ctx) const;
	void render_header(const Renderer& renderer) const;
	// The height of the panels between the header and the hint bar. Input and drawing both
	// read it, so a click and the row drawn under it come from one row count.
	//
	// Example:
	//   body_height(renderer);   // a 900-pixel screen -> 824, after a 48 header and a 28 hint bar
	[[nodiscard]] static int body_height(const Renderer& renderer);
	void render_list(const Renderer& renderer, const SpellRegistry& spells) const;
	void render_fields(const Renderer& renderer) const;
	void render_hint(const Renderer& renderer) const;

	[[nodiscard]] const std::string& current_key() const;
	[[nodiscard]] FieldId current_field() const;

	[[nodiscard]] std::string field_label(FieldId field) const;
	[[nodiscard]] std::string field_value(FieldId field) const;

	void field_adjust(FieldId field, int delta);
	void field_cycle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);

public:
	// Every field, in FieldId order: how it is edited, its label, and a plain number's member and bounds.
	static constexpr std::array<Descriptor, static_cast<size_t>(FieldId::COUNT)> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		Descriptor{ FieldId::LEVEL, FieldKind::ADJUST, "Level", std::nullopt },
		Descriptor{ FieldId::CLASS, FieldKind::TOGGLE, "Class", std::nullopt },
		Descriptor{ FieldId::EFFECT_TYPE, FieldKind::TOGGLE, "Effect", std::nullopt },
		Descriptor{ FieldId::DESCRIPTION, FieldKind::TEXT, "Description", std::nullopt },
	} };

	void enter(const SpellRegistry& spells);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
