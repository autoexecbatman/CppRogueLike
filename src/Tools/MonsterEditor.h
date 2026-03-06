// file: MonsterEditor.h
#pragma once

// Full-screen in-game monster data editor.
// Entered from main menu ("Monster Editor" option).
// Reads and writes data/content/monsters.json via MonsterCreator.
//
// Controls:
//   Tab              -- switch focus: list panel <-> field panel
//   Up / Down        -- navigate monster list or field list
//   Left / Right     -- adjust integer field value (+/-1)
//   Ctrl+Left/Right  -- adjust integer field value (+/-10)
//   Enter            -- edit string field / toggle bool / open tile picker
//   F2               -- open tile picker for current monster tile
//   Ctrl+S           -- save monsters.json
//   Esc              -- commit edits and exit to main menu

#include <string>
#include <vector>

#include "../Factories/MonsterCreator.h"
#include "../Renderer/Renderer.h"

struct GameContext;

class MonsterEditor
{
public:
	void enter();
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return m_active; }

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
		CORPSE,
		HP_NUM,
		HP_SIDES,
		HP_BONUS,
		THACO,
		AC,
		XP,
		DR,
		STR_NUM,
		STR_SIDES,
		STR_BONUS,
		DEX_NUM,
		DEX_SIDES,
		DEX_BONUS,
		CON_NUM,
		CON_SIDES,
		CON_BONUS,
		INT_NUM,
		INT_SIDES,
		INT_BONUS,
		WIS_NUM,
		WIS_SIDES,
		WIS_BONUS,
		CHA_NUM,
		CHA_SIDES,
		CHA_BONUS,
		WEAPON,
		DMG_MIN,
		DMG_MAX,
		DMG_DISPLAY,
		AI_TYPE,
		CAN_SWIM,
		WEIGHT,
		DEPTH_MIN,
		DEPTH_MAX,
		TILE,
		COUNT
	};

	static constexpr int FIELD_COUNT = static_cast<int>(FieldId::COUNT);

	bool m_active{ false };
	Mode m_mode{ Mode::NORMAL };
	int m_focus{ 0 }; // 0 = list panel, 1 = field panel

	// Monster list (string keys: builtins, custom, class-based)
	std::vector<std::string> m_keys;
	int m_list_cursor{ 0 };
	int m_list_scroll{ 0 };

	// Working copy of selected monster's params
	MonsterParams m_working{};
	bool m_is_class_based{ false };

	// Field navigation
	int m_field_cursor{ 0 };
	int m_field_scroll{ 0 };
	std::string m_edit_buf;

	// Tile picker state
	int m_picker_sheet{ 0 };
	int m_picker_scroll{ 0 };

	// Status feedback
	double m_last_save_time{ -100.0 };

	// Layout constants (pixels)
	static constexpr int LIST_W = 220;
	static constexpr int HEADER_H = 48;
	static constexpr int HINT_H = 28;
	static constexpr int FIELD_H = 26;

	void load_working();
	void commit_working();

	void handle_input(GameContext& ctx);
	void handle_normal(const GameContext& ctx);
	void handle_edit_string();
	void handle_picker(const Renderer& r);

	void render(const GameContext& ctx) const;
	void render_header(const Renderer& r) const;
	void render_list(const Renderer& r) const;
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
};
