// file: SpellEditor.h
#pragma once

// Full-screen in-game spell metadata editor.
// Entered from main menu ("Spell Editor" option).
// Reads and writes data/content/spells.json via SpellSystem.
//
// Controls:
//   Tab          -- switch focus: list <-> fields
//   Up / Down    -- navigate spell list or field list
//   Left / Right -- adjust Level field (+/-1)
//   Enter        -- edit string field / cycle class field
//   Ctrl+S       -- save spells.json
//   Esc          -- commit edits and exit to main menu

#include <string>
#include <vector>

#include "../Systems/SpellSystem.h"

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
	bool active{ false };
	Mode mode{ Mode::NORMAL };
	int focus{ 0 }; // 0 = list, 1 = fields

	// Spell list (string keys: builtins then custom)
	std::vector<std::string> keys;
	int listCursor{ 0 };
	int listScroll{ 0 };

	// Working copy
	SpellDefinition working{};

	// Field navigation
	int fieldCursor{ 0 };
	std::string editBuffer;

	// Status feedback
	double lastSaveTime{ -100.0 };

	void load_working();
	void commit_working();

	void handle_input(GameContext& ctx);
	void handle_normal(const GameContext& ctx);
	void handle_edit_string();

	void render(const GameContext& ctx) const;
	void render_header(const Renderer& r) const;
	void render_list(const Renderer& r) const;
	void render_fields(const Renderer& r) const;
	void render_hint(const Renderer& r) const;

	[[nodiscard]] const std::string& current_key() const;
	[[nodiscard]] FieldId current_field() const;

	[[nodiscard]] std::string field_label(FieldId f) const;
	[[nodiscard]] std::string field_value(FieldId f) const;
	[[nodiscard]] bool field_is_string(FieldId f) const;

	void field_adjust(FieldId f, int delta);
	void field_cycle(FieldId f);
	void field_set_string(FieldId f, std::string val);

public:
	void enter();
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }

};
