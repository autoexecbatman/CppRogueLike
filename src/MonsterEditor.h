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

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "EditorField.h"
#include "ListCursor.h"
#include "MonsterRegistry.h"

struct GameContext;
class Renderer;

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

	// A plain number field's member and bounds, read from the working MonsterParams.
	using Plain = PlainNumber<MonsterParams>;
	// One field's entry in FIELDS.
	using Descriptor = FieldDescriptor<FieldId, MonsterParams>;

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

	// Which of the working monster's innate powers the spell and frequency fields edit.
	size_t selectedPower{ 0 };

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

	void field_adjust(FieldId field, int delta);
	void field_toggle(FieldId field);
	void field_set_string(FieldId field, std::string newValue);

public:
	// Every field, in FieldId order: how it is edited, its label, and a plain number's member and bounds.
	static constexpr std::array<Descriptor, FIELD_COUNT> FIELDS{ {
		Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
		Descriptor{ FieldId::CORPSE, FieldKind::TEXT, "Corpse", std::nullopt },
		Descriptor{ FieldId::CORPSE_NUTRITION, FieldKind::ADJUST, "Corpse Nutrition", Plain{ &MonsterParams::corpseNutrition, 0, 9999 } },
		Descriptor{ FieldId::CORPSE_TASTE, FieldKind::TEXT, "Corpse Taste", std::nullopt },
		Descriptor{ FieldId::CORPSE_EDIBLE, FieldKind::TOGGLE, "Corpse Edible", std::nullopt },
		Descriptor{ FieldId::HD_LOWEST, FieldKind::ADJUST, "HD Lowest", std::nullopt },
		Descriptor{ FieldId::HD_HIGHEST, FieldKind::ADJUST, "HD Highest", std::nullopt },
		Descriptor{ FieldId::HD_SIDES, FieldKind::ADJUST, "HD Sides", std::nullopt },
		Descriptor{ FieldId::HD_BONUS, FieldKind::ADJUST, "HD Bonus", std::nullopt },
		Descriptor{ FieldId::THACO, FieldKind::ADJUST, "THAC0", Plain{ &MonsterParams::thaco, 1, 25 } },
		Descriptor{ FieldId::AC, FieldKind::ADJUST, "AC", Plain{ &MonsterParams::ac, -10, 10 } },
		Descriptor{ FieldId::XP_COUNT, FieldKind::ADJUST, "XP For HD", std::nullopt },
		Descriptor{ FieldId::XP, FieldKind::ADJUST, "XP", std::nullopt },
		Descriptor{ FieldId::DR, FieldKind::ADJUST, "DR", Plain{ &MonsterParams::dr, 0, 20 } },
		Descriptor{ FieldId::STR_SOURCE, FieldKind::TOGGLE, "Strength", std::nullopt },
		Descriptor{ FieldId::STR_SCORE, FieldKind::ADJUST, "STR Score", std::nullopt },
		Descriptor{ FieldId::STR_EXCEPTIONAL, FieldKind::ADJUST, "STR Exceptional", std::nullopt },
		Descriptor{ FieldId::WEAPON, FieldKind::TEXT, "Natural Attack", std::nullopt },
		Descriptor{ FieldId::DMG_NUM, FieldKind::ADJUST, "Dmg Num", std::nullopt },
		Descriptor{ FieldId::DMG_SIDES, FieldKind::ADJUST, "Dmg Sides", std::nullopt },
		Descriptor{ FieldId::DMG_BONUS, FieldKind::ADJUST, "Dmg Bonus", std::nullopt },
		Descriptor{ FieldId::DMG_TYPE, FieldKind::TOGGLE, "Dmg Type", std::nullopt },
		Descriptor{ FieldId::AI_TYPE, FieldKind::TOGGLE, "AI Type", std::nullopt },
		Descriptor{ FieldId::ETHICS, FieldKind::TOGGLE, "Ethics", std::nullopt },
		Descriptor{ FieldId::MORALITY, FieldKind::TOGGLE, "Morality", std::nullopt },
		Descriptor{ FieldId::SIZE, FieldKind::TOGGLE, "Size", std::nullopt },
		Descriptor{ FieldId::INTELLIGENCE, FieldKind::TOGGLE, "Intelligence", std::nullopt },
		Descriptor{ FieldId::CAN_SWIM, FieldKind::TOGGLE, "Can Swim", std::nullopt },
		Descriptor{ FieldId::CAN_FLY, FieldKind::TOGGLE, "Can Fly", std::nullopt },
		Descriptor{ FieldId::WALKS_WEBS, FieldKind::TOGGLE, "Walks Webs", std::nullopt },
		Descriptor{ FieldId::ATTITUDE, FieldKind::TOGGLE, "Attitude", std::nullopt },
		Descriptor{ FieldId::DISPLACEABLE, FieldKind::TOGGLE, "Displaceable", std::nullopt },
		Descriptor{ FieldId::CASTER_CLASS, FieldKind::TOGGLE, "Caster Class", std::nullopt },
		Descriptor{ FieldId::CASTER_LEVEL, FieldKind::ADJUST, "Caster Level", std::nullopt },
		Descriptor{ FieldId::INNATE_POWER, FieldKind::ADJUST, "Innate Power", std::nullopt },
		Descriptor{ FieldId::INNATE_SPELL, FieldKind::TEXT, "Power Spell", std::nullopt },
		Descriptor{ FieldId::INNATE_FREQUENCY, FieldKind::TOGGLE, "Power Frequency", std::nullopt },
		Descriptor{ FieldId::WEIGHT, FieldKind::ADJUST, "Spawn Weight", Plain{ &MonsterParams::baseWeight, 0, 100 } },
		Descriptor{ FieldId::DEPTH_MIN, FieldKind::ADJUST, "Depth Min", Plain{ &MonsterParams::levelMinimum, 1, 20 } },
		Descriptor{ FieldId::DEPTH_MAX, FieldKind::ADJUST, "Depth Max", Plain{ &MonsterParams::levelMaximum, 0, 20 } },
		Descriptor{ FieldId::TILE, FieldKind::TILE, "Tile", std::nullopt },
	} };

	void enter(const MonsterRegistry& monsters);
	void exit(GameContext& ctx);
	void tick(GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }
};
