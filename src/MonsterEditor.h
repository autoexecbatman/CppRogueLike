// file: MonsterEditor.h
#pragma once

// Full-screen in-game monster data editor.
// Entered from main menu ("Monster Editor" option).
// Edits the MonsterRegistry in ctx and saves it to data/content/monsters.json.
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

#include "ListCursor.h"
#include "MonsterRegistry.h"
#include "Renderer.h"

struct GameContext;

class MonsterEditor
{
private:
	enum class Mode
	{
		NORMAL,
		EDIT_STRING,
		TILE_PICKER
	};

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
		WEIGHT,
		DEPTH_MIN,
		DEPTH_MAX,
		TILE,
		COUNT
	};

	static constexpr size_t FIELD_COUNT = static_cast<size_t>(FieldId::COUNT);

	bool active{ false };
	Mode mode{ Mode::NORMAL };
	int focus{ 0 }; // 0 = list panel, 1 = field panel

	// Monster list (string keys: builtins, then custom)
	std::vector<std::string> keys;
	ListCursor listCursor{};

	// Working copy of selected monster's params
	MonsterParams working{};

	// Which of the working monster's hit dice counts the XP field edits.
	size_t selectedXpCount{ 0 };

	// Field navigation
	ListCursor fieldCursor{};
	std::string editBuffer;

	// Tile picker state
	int pickerSheet{ 0 };
	int pickerScroll{ 0 };

	// Status feedback
	double lastSaveTime{ -100.0 };

	void load_working(const MonsterRegistry& monsters);
	void commit_working(MonsterRegistry& monsters);

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
	void render_list(const Renderer& renderer, const MonsterRegistry& monsters) const;
	void render_fields(const Renderer& renderer) const;
	void render_picker(const Renderer& renderer) const;
	void render_hint(const Renderer& renderer) const;

	[[nodiscard]] const std::string& current_key() const;
	[[nodiscard]] FieldId current_field() const;

	[[nodiscard]] std::string field_label(FieldId field) const;
	// Rebuilds the working damage from dice, so its text and its range follow.
	void set_damage_dice(int num, int sides, int bonus);

	[[nodiscard]] std::string field_value(FieldId field) const;
	[[nodiscard]] bool field_is_string(FieldId field) const;
	[[nodiscard]] bool field_is_toggle(FieldId field) const;

	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);

public:
	void enter(const MonsterRegistry& monsters);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
