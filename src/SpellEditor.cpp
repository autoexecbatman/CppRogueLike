// file: SpellEditor.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "GameContext.h"
#include "Menu.h"
#include "Paths.h"
#include "Renderer.h"
#include "SpellEditor.h"
#include "SpellRegistry.h"

static_assert(is_well_formed_table(SpellEditor::FIELDS), "SpellEditor::FIELDS lists a field out of FieldId order, or a plain number on a field that is not ADJUST");

namespace
{

std::string effect_type_name(SpellEffectType effectType)
{
	switch (effectType)
	{
	case SpellEffectType::CURE_LIGHT_WOUNDS:
	{
		return "Cure Light Wounds";
	}
	case SpellEffectType::BLESS:
	{
		return "Bless";
	}
	case SpellEffectType::SANCTUARY:
	{
		return "Sanctuary";
	}
	case SpellEffectType::HOLD_PERSON:
	{
		return "Hold Person";
	}
	case SpellEffectType::SILENCE:
	{
		return "Silence";
	}
	case SpellEffectType::MAGIC_MISSILE:
	{
		return "Magic Missile";
	}
	case SpellEffectType::SHIELD:
	{
		return "Shield";
	}
	case SpellEffectType::SLEEP:
	{
		return "Sleep";
	}
	case SpellEffectType::INVISIBILITY:
	{
		return "Invisibility";
	}
	case SpellEffectType::WEB:
	{
		return "Web";
	}
	case SpellEffectType::FIREBALL:
	{
		return "Fireball";
	}
	case SpellEffectType::TELEPORT:
	{
		return "Teleport";
	}
	default:
	{
		return "None";
	}
	}
}

SpellEffectType effect_type_next(SpellEffectType effectType)
{
	int value = static_cast<int>(effectType) + 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (value > LAST)
	{
		value = 0;
	}
	return static_cast<SpellEffectType>(value);
}

SpellEffectType effect_type_prev(SpellEffectType effectType)
{
	int value = static_cast<int>(effectType) - 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (value < 0)
	{
		value = LAST;
	}
	return static_cast<SpellEffectType>(value);
}

} // namespace

constexpr int LIST_WIDTH = 220;
constexpr int HEADER_HEIGHT = 48;
constexpr int HINT_HEIGHT = 28;
constexpr int FIELD_HEIGHT = 30;
constexpr int ITEM_HEIGHT = 32;
constexpr int LIST_PAD = 8;
constexpr size_t FIELD_COUNT = static_cast<size_t>(FieldId::COUNT);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SpellEditor::enter(const SpellRegistry& spells)
{
	active = true;
	mode = Mode::NORMAL;
	focus = 0;

	keys = spells.get_all_keys();

	listCursor.reset(keys.size());
	fieldCursor.reset(FIELD_COUNT);

	load_working(spells);
}

void SpellEditor::exit(GameContext& ctx)
{
	assert(ctx.spellRegistry && "SpellEditor::exit: called without a spellRegistry");

	commit_working(*ctx.spellRegistry);
	active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void SpellEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	render(ctx);
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void SpellEditor::load_working(const SpellRegistry& spells)
{
	if (keys.empty())
	{
		return;
	}

	working = spells.get_by_key(current_key());
	fieldCursor.reset(FIELD_COUNT);
	mode = Mode::NORMAL;
	editBuffer.clear();
}

void SpellEditor::commit_working(SpellRegistry& spells)
{
	if (keys.empty())
	{
		return;
	}

	spells.set_by_key(current_key(), working);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void SpellEditor::handle_input(GameContext& ctx)
{
	assert(ctx.spellRegistry && "SpellEditor::handle_input: called without a spellRegistry");

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (ctrl && IsKeyPressed(KEY_S))
	{
		commit_working(*ctx.spellRegistry);
		ctx.spellRegistry->save(Paths::SPELLS);
		lastSaveTime = GetTime();
		return;
	}

	if (IsKeyPressed(KEY_TAB))
	{
		focus = 1 - focus;
		return;
	}

	if (mode == Mode::NORMAL)
	{
		handle_normal(ctx);
	}
	else if (mode == Mode::EDIT_STRING)
	{
		handle_edit_string();
	}
}

void SpellEditor::handle_normal(const GameContext& ctx)
{
	assert(ctx.spellRegistry && "SpellEditor::handle_normal: called without a spellRegistry");

	SpellRegistry& spells = *ctx.spellRegistry;
	const int bodyHeight = body_height(*ctx.renderer);
	const size_t visibleListRows = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t visibleFields = whole_rows_within(bodyHeight, FIELD_HEIGHT);
	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	// A click in the header is on neither panel.
	if (clicked && mouse.y >= HEADER_HEIGHT)
	{
		const int pixelsBelowTop = static_cast<int>(mouse.y) - HEADER_HEIGHT;
		if (mouse.x < LIST_WIDTH)
		{
			focus = 0;
			const std::optional<size_t> row = listCursor.row_under(whole_rows_within(pixelsBelowTop, ITEM_HEIGHT), visibleListRows);
			if (row && *row != listCursor.current_row())
			{
				commit_working(spells);
				listCursor.select(*row);
				load_working(spells);
			}
		}
		else
		{
			focus = 1;
			const std::optional<size_t> field = fieldCursor.row_under(whole_rows_within(pixelsBelowTop, FIELD_HEIGHT), visibleFields);
			if (field)
			{
				fieldCursor.select(*field);
			}
		}
	}

	// Mouse wheel
	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f)
	{
		if (mouse.x < LIST_WIDTH)
		{
			listCursor.scroll_by(-static_cast<int>(wheel), visibleListRows);
		}
		else
		{
			fieldCursor.scroll_by(-static_cast<int>(wheel), visibleFields);
		}
	}

	if (focus == 0)
	{
		if (IsKeyPressed(KEY_UP) && listCursor.can_move_up())
		{
			commit_working(spells);
			listCursor.move_up();
			load_working(spells);
		}
		// A screen too short for one row has nowhere to scroll the cursor to.
		else if (IsKeyPressed(KEY_DOWN) && listCursor.can_move_down() && visibleListRows > 0)
		{
			commit_working(spells);
			listCursor.move_down(visibleListRows);
			load_working(spells);
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working(spells);
			SpellDefinition definition;
			definition.name = "New Spell";
			definition.level = 1;
			definition.spellClass = SpellClass::WIZARD;
			definition.description = "No effect yet.";
			const std::string newKey = spells.add_custom(std::move(definition));
			keys = spells.get_all_keys();
			listCursor.set_row_count(keys.size());
			for (size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex)
			{
				if (keys[keyIndex] == newKey)
				{
					listCursor.select(keyIndex);
					break;
				}
			}
			load_working(spells);
			focus = 1;
		}
		else if (IsKeyPressed(KEY_DELETE))
		{
			if (!spells.is_builtin_key(current_key()))
			{
				spells.remove_custom(current_key());
				keys = spells.get_all_keys();
				listCursor.set_row_count(keys.size());
				load_working(spells);
			}
		}
		return;
	}

	// Field panel navigation
	if (IsKeyPressed(KEY_UP) && fieldCursor.can_move_up())
	{
		fieldCursor.move_up();
	}
	else if (IsKeyPressed(KEY_DOWN) && fieldCursor.can_move_down() && visibleFields > 0)
	{
		fieldCursor.move_down(visibleFields);
	}

	FieldId fieldId = current_field();
	const FieldKind kind = descriptor_of(FIELDS, fieldId).kind;

	if (IsKeyPressed(KEY_ENTER))
	{
		if (kind == FieldKind::TEXT)
		{
			editBuffer = field_value(fieldId);
			mode = Mode::EDIT_STRING;
		}
		else
		{
			field_cycle(fieldId);
		}
		return;
	}

	if (kind != FieldKind::TEXT)
	{
		if (IsKeyPressed(KEY_LEFT))
		{
			field_adjust(fieldId, -1);
		}
		else if (IsKeyPressed(KEY_RIGHT))
		{
			field_adjust(fieldId, 1);
		}
	}
}

void SpellEditor::handle_edit_string()
{
	int pressed = GetCharPressed();
	while (pressed != 0)
	{
		if (pressed >= 32 && pressed < 127)
		{
			editBuffer += static_cast<char>(pressed);
		}
		pressed = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE) && !editBuffer.empty())
	{
		editBuffer.pop_back();
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		field_set_string(current_field(), editBuffer);
		mode = Mode::NORMAL;
	}

	if (IsKeyPressed(KEY_ESCAPE))
	{
		mode = Mode::NORMAL;
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void SpellEditor::render(const GameContext& ctx) const
{
	assert(ctx.spellRegistry && "SpellEditor::render: called without a spellRegistry");

	const Renderer& renderer = *ctx.renderer;
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(renderer);
	render_list(renderer, *ctx.spellRegistry);
	render_fields(renderer);
	render_hint(renderer);
}

void SpellEditor::render_header(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 20, 20, 40, 255 });
	renderer.draw_text_color(Vector2D{ 8, 6 }, "SPELL EDITOR", Color{ 180, 255, 180, 255 });
	renderer.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Ctrl+S:save  Esc:exit",
		Color{ 130, 130, 100, 255 });
}

int SpellEditor::body_height(const Renderer& renderer)
{
	return renderer.get_screen_height() - HEADER_HEIGHT - HINT_HEIGHT;
}

void SpellEditor::render_list(const Renderer& renderer, const SpellRegistry& spells) const
{
	int bodyY = HEADER_HEIGHT;
	int bodyHeight = body_height(renderer);

	DrawRectangle(0, bodyY, LIST_WIDTH, bodyHeight, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_WIDTH, bodyY, LIST_WIDTH, bodyY + bodyHeight, Color{ 80, 80, 120, 255 });

	const size_t visibleCount = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t scroll = listCursor.first_visible(visibleCount);

	::Vector2 mouse = GetMousePosition();

	for (size_t spellIndex = scroll; spellIndex < keys.size(); ++spellIndex)
	{
		int itemY = bodyY + static_cast<int>(spellIndex - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > bodyY + bodyHeight)
		{
			break;
		}

		bool isSelected = (spellIndex == listCursor.current_row());
		bool hovered = mouse.x >= 0 && mouse.x < LIST_WIDTH && mouse.y >= itemY && mouse.y < itemY + ITEM_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (isSelected && focus == 0)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (isSelected)
		{
			bgColor = Color{ 0, 40, 0, 150 };
		}
		else if (hovered)
		{
			bgColor = Color{ 30, 30, 30, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		// Show spell class indicator
		const SpellDefinition& definition = isSelected
			? working
			: spells.get_by_key(keys[spellIndex]);

		const bool isCustom = !spells.is_builtin_key(keys[spellIndex]);

		Color classColour = (definition.spellClass == SpellClass::CLERIC)
			? Color{ 100, 220, 255, 255 }
			: Color{ 255, 160, 80, 255 };

		std::string label = std::format("[{}] {}{}", definition.level, definition.name, isCustom ? " *" : "");
		Color textColor = isSelected ? Color{ 180, 255, 180, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - 16) / 2 }, label, textColor);

		// Class dot on right edge
		const char* classTag = (definition.spellClass == SpellClass::CLERIC) ? "C" : "W";
		renderer.draw_text_color(Vector2D{ LIST_WIDTH - 20, itemY + (ITEM_HEIGHT - 16) / 2 }, classTag, classColour);
	}
}

void SpellEditor::render_fields(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = body_height(renderer);

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 8, 8, 16, 255 });

	::Vector2 mouse = GetMousePosition();

	const size_t scroll = fieldCursor.first_visible(whole_rows_within(fieldsHeight, FIELD_HEIGHT));
	for (size_t fieldIndex = scroll; fieldIndex < FIELD_COUNT; ++fieldIndex)
	{
		int itemY = panelY + static_cast<int>(fieldIndex - scroll) * FIELD_HEIGHT;
		if (itemY + FIELD_HEIGHT > panelY + fieldsHeight)
		{
			break;
		}

		FieldId fieldId = static_cast<FieldId>(fieldIndex);
		bool isSelected = (fieldIndex == fieldCursor.current_row());
		bool hovered = mouse.x >= panelX && mouse.x < screenWidth && mouse.y >= itemY && mouse.y < itemY + FIELD_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (isSelected && focus == 1)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (isSelected)
		{
			bgColor = Color{ 0, 40, 0, 140 };
		}
		else if (hovered)
		{
			bgColor = Color{ 20, 30, 20, 120 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(panelX, itemY, fieldsWidth, FIELD_HEIGHT, bgColor);
		}

		Color labelColor = isSelected ? Color{ 180, 255, 180, 255 } : Color{ 150, 150, 150, 255 };
		Color valueColor = isSelected ? Color{ 255, 255, 200, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fieldId), labelColor);

		std::string valueText;
		if (isSelected && mode == Mode::EDIT_STRING)
		{
			valueText = editBuffer + "_";
		}
		else
		{
			valueText = field_value(fieldId);
		}

		int valuePixelX = screenWidth - renderer.measure_text(valueText) - 16;
		renderer.draw_text_color(Vector2D{ valuePixelX, itemY + (FIELD_HEIGHT - 16) / 2 }, valueText, valueColor);
	}
}

void SpellEditor::render_hint(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int hintY = screenHeight - HINT_HEIGHT;

	DrawRectangle(0, hintY, screenWidth, HINT_HEIGHT, Color{ 20, 20, 40, 255 });

	bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;
	std::string hintText;

	if (isSaveRecent)
	{
		hintText = "Saved!";
	}
	else if (mode == Mode::EDIT_STRING)
	{
		hintText = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (focus == 0)
	{
		hintText = "[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit";
	}
	else
	{
		hintText = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hintColor = isSaveRecent ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	renderer.draw_text_color(Vector2D{ 8, hintY + 6 }, hintText, hintColor);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& SpellEditor::current_key() const
{
	if (keys.empty())
	{
		throw std::out_of_range("SpellEditor::current_key -- key list is empty");
	}
	return keys[listCursor.current_row()];
}

FieldId SpellEditor::current_field() const
{
	return static_cast<FieldId>(fieldCursor.current_row());
}

std::string SpellEditor::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

std::string SpellEditor::field_value(FieldId field) const
{
	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::LEVEL:
	{
		return std::format("{}", working.level);
	}
	case FieldId::CLASS:
	{
		if (working.spellClass == SpellClass::CLERIC)
		{
			return "cleric";
		}
		if (working.spellClass == SpellClass::WIZARD)
		{
			return "wizard";
		}
		return "both";
	}
	case FieldId::EFFECT_TYPE:
	{
		return effect_type_name(working.effect_type);
	}
	case FieldId::DESCRIPTION:
	{
		return working.description;
	}
	default:
	{
		refuse_misrouted_field(descriptor_of(FIELDS, field).label, "SpellEditor::field_value");
	}
	}
}

void SpellEditor::field_adjust(FieldId field, int delta)
{
	if (field == FieldId::LEVEL)
	{
		working.level = std::clamp(working.level + delta, 1, 9);
	}
	else if (field == FieldId::EFFECT_TYPE)
	{
		if (delta > 0)
		{
			working.effect_type = effect_type_next(working.effect_type);
		}
		else
		{
			working.effect_type = effect_type_prev(working.effect_type);
		}
	}
}

void SpellEditor::field_cycle(FieldId field)
{
	if (field == FieldId::CLASS)
	{
		if (working.spellClass == SpellClass::CLERIC)
		{
			working.spellClass = SpellClass::WIZARD;
		}
		else if (working.spellClass == SpellClass::WIZARD)
		{
			working.spellClass = SpellClass::BOTH;
		}
		else
		{
			working.spellClass = SpellClass::CLERIC;
		}
	}
	else if (field == FieldId::EFFECT_TYPE)
	{
		working.effect_type = effect_type_next(working.effect_type);
	}
}

void SpellEditor::field_set_string(FieldId field, std::string newValue)
{
	if (field == FieldId::NAME)
	{
		working.name = std::move(newValue);
	}
	else if (field == FieldId::DESCRIPTION)
	{
		working.description = std::move(newValue);
	}
	else
	{
		refuse_misrouted_field(descriptor_of(FIELDS, field).label, "SpellEditor::field_set_string");
	}
}

// end of file: SpellEditor.cpp
