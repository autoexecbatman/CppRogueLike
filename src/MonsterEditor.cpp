// file: MonsterEditor.cpp
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "Ai.h"
#include "Attitude.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "IntelligenceRating.h"
#include "Menu.h"
#include "MonsterEditor.h"
#include "MonsterRegistry.h"
#include "MonsterStrength.h"
#include "Paths.h"
#include "Renderer.h"
#include "SpellRegistry.h"
#include "Spellcasting.h"

constexpr int LIST_WIDTH = 220;
constexpr int HEADER_HEIGHT = 48;
constexpr int HINT_HEIGHT = 28;
constexpr int FIELD_HEIGHT = 26;
constexpr int ITEM_HEIGHT = 30;
constexpr int LIST_TILE_SIZE = 22;
constexpr int LIST_PAD = 6;
constexpr int PICKER_PAD = 10;
constexpr int PICKER_SUB_HEADER_HEIGHT = 28;
constexpr int PICKER_TILE_TARGET = 64; // drawn cell size the grid aims for

static_assert(
	is_well_formed_table(MonsterEditor::FIELDS),
	"MonsterEditor::FIELDS is out of order or misplaces a plain number");

// Largest integer multiple of the sheet's own cell size that fits the target,
// so a 16-pixel tile draws at 4x and a 64-pixel tile at 1x - never blurred.
namespace
{
int picker_tile_size(const Renderer& renderer, int sheetIndex)
{
	const int cellSize = renderer.get_sheet_cell_size(static_cast<TileSheet>(sheetIndex));
	return cellSize * std::max(1, PICKER_TILE_TARGET / cellSize);
}
} // namespace

namespace
{

// Alignment fields cycle through their axis rather than being typed, so each
// press moves one step and wraps at the end. The names come from Alignment.h,
// beside the enums, so the editor shows what a record carries.
Ethics next_ethics(Ethics ethics)
{
	switch (ethics)
	{
	case Ethics::LAWFUL:
	{
		return Ethics::NEUTRAL;
	}
	case Ethics::NEUTRAL:
	{
		return Ethics::CHAOTIC;
	}
	case Ethics::CHAOTIC:
	{
		return Ethics::LAWFUL;
	}
	}
	return Ethics::NEUTRAL;
}

Morality next_morality(Morality morality)
{
	switch (morality)
	{
	case Morality::GOOD:
	{
		return Morality::NEUTRAL;
	}
	case Morality::NEUTRAL:
	{
		return Morality::EVIL;
	}
	case Morality::EVIL:
	{
		return Morality::GOOD;
	}
	}
	return Morality::NEUTRAL;
}

std::string prettify_key(std::string_view key)
{
	std::string result;
	bool capNext = true;
	for (char character : key)
	{
		if (character == '_')
		{
			result += ' ';
			capNext = true;
		}
		else if (capNext)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			capNext = false;
		}
		else
		{
			result += character;
		}
	}
	return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MonsterEditor::enter(const MonsterRegistry& monsters)
{
	active = true;
	mode = Mode::NORMAL;
	focus = 0;

	keys = monsters.get_all_keys();

	listCursor.reset(keys.size());
	fieldCursor.reset(FIELD_COUNT);
	pickerSheet = 0;
	pickerScroll = 0;

	load_working(monsters);
}

void MonsterEditor::exit(GameContext& ctx)
{
	assert(ctx.monsterRegistry && "MonsterEditor::exit: called without a monsterRegistry");

	commit_working(*ctx.monsterRegistry);
	active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void MonsterEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	render(ctx);
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void MonsterEditor::load_working(const MonsterRegistry& monsters)
{
	if (keys.empty())
	{
		return;
	}

	const std::string& key = current_key();
	working = monsters.get_params(key);
	selectedXpCount = 0;
	selectedPower = 0;
	fieldCursor.reset(FIELD_COUNT);
	mode = Mode::NORMAL;
	editBuffer.clear();
}

void MonsterEditor::commit_working(MonsterRegistry& monsters)
{
	if (keys.empty())
	{
		return;
	}
	monsters.set_params(current_key(), working);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void MonsterEditor::handle_input(GameContext& ctx)
{
	assert(ctx.monsterRegistry && "MonsterEditor::handle_input: called without a monsterRegistry");

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (ctrl && IsKeyPressed(KEY_S))
	{
		commit_working(*ctx.monsterRegistry);
		ctx.monsterRegistry->save(Paths::MONSTERS);
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
	else if (mode == Mode::TILE_PICKER)
	{
		handle_picker(*ctx.renderer);
	}
}

void MonsterEditor::handle_normal(const GameContext& ctx)
{
	assert(ctx.monsterRegistry && "MonsterEditor::handle_normal: called without a monsterRegistry");

	MonsterRegistry& monsters = *ctx.monsterRegistry;
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	const Renderer& renderer = *ctx.renderer;
	int bodyHeight = body_height(renderer);
	const size_t visibleListRows = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t visibleFields = whole_rows_within(bodyHeight, FIELD_HEIGHT);

	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	// Mouse: click selects panel and item. A click in the header is on neither panel.
	if (clicked && mouse.y >= HEADER_HEIGHT)
	{
		const int pixelsBelowTop = static_cast<int>(mouse.y) - HEADER_HEIGHT;
		if (mouse.x < LIST_WIDTH)
		{
			focus = 0;
			const std::optional<size_t> row = listCursor.row_under(whole_rows_within(pixelsBelowTop, ITEM_HEIGHT), visibleListRows);
			if (row && *row != listCursor.current_row())
			{
				commit_working(monsters);
				listCursor.select(*row);
				load_working(monsters);
			}
		}
		else if (mode == Mode::NORMAL)
		{
			focus = 1;
			const std::optional<size_t> field = fieldCursor.row_under(whole_rows_within(pixelsBelowTop, FIELD_HEIGHT), visibleFields);
			if (field)
			{
				fieldCursor.select(*field);
			}
		}
	}

	// Mouse wheel scrolling
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

	// Keyboard: list panel
	if (focus == 0)
	{
		if (IsKeyPressed(KEY_UP) && listCursor.can_move_up())
		{
			commit_working(monsters);
			listCursor.move_up();
			load_working(monsters);
		}
		// A screen too short for one row has nowhere to scroll the cursor to.
		else if (IsKeyPressed(KEY_DOWN) && listCursor.can_move_down() && visibleListRows > 0)
		{
			commit_working(monsters);
			listCursor.move_down(visibleListRows);
			load_working(monsters);
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working(monsters);
			MonsterParams defaults;
			defaults.name = "New Monster";
			defaults.corpseName = "dead monster";
			defaults.corpseNutrition = 40;
			defaults.corpseTaste = "It tastes like nothing you have eaten before.";
			defaults.hitDice = MonsterHitDice{ 8, 0, { { 1, 1 } } };
			defaults.thaco = 20;
			defaults.ac = 10;
			defaults.strength = std::nullopt;
			defaults.naturalAttack = "claws";
			defaults.damage = DamageInfo{ DiceExpr{ 1, 4, 0 }, DamageType::PHYSICAL };
			defaults.baseWeight = 10;
			defaults.levelMinimum = 1;
			defaults.levelMaximum = 5;
			const std::string newKey = monsters.add_custom(std::move(defaults));
			keys = monsters.get_all_keys();
			listCursor.set_row_count(keys.size());
			for (size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex)
			{
				if (keys[keyIndex] == newKey)
				{
					listCursor.select(keyIndex);
					break;
				}
			}
			load_working(monsters);
			focus = 1;
		}
		else if (IsKeyPressed(KEY_DELETE))
		{
			if (!monsters.is_builtin(current_key()))
			{
				monsters.remove_custom(current_key());
				keys = monsters.get_all_keys();
				listCursor.set_row_count(keys.size());
				load_working(monsters);
			}
		}
		return;
	}

	// Keyboard: field panel
	if (IsKeyPressed(KEY_UP) && fieldCursor.can_move_up())
	{
		fieldCursor.move_up();
	}
	else if (IsKeyPressed(KEY_DOWN) && fieldCursor.can_move_down() && visibleFields > 0)
	{
		fieldCursor.move_down(visibleFields);
	}
	else if (IsKeyPressed(KEY_LEFT) && focus == 0)
	{
		focus = 0;
	}

	FieldId fieldId = current_field();

	const FieldKind kind = descriptor_of(FIELDS, fieldId).kind;

	const bool isPickerKey = IsKeyPressed(KEY_F2);
	const bool isTileField = kind == FieldKind::TILE;
	const bool isTileChosen = isTileField && IsKeyPressed(KEY_ENTER);
	if (isPickerKey || isTileChosen)
	{
		mode = Mode::TILE_PICKER;
		return;
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		if (kind == FieldKind::TEXT)
		{
			editBuffer = field_value(fieldId);
			mode = Mode::EDIT_STRING;
		}
		else if (kind == FieldKind::TOGGLE)
		{
			field_toggle(fieldId);
		}
		return;
	}

	if (kind == FieldKind::ADJUST)
	{
		int delta = ctrl ? 10 : 1;
		if (IsKeyPressed(KEY_LEFT))
		{
			field_adjust(fieldId, -delta);
		}
		else if (IsKeyPressed(KEY_RIGHT))
		{
			field_adjust(fieldId, delta);
		}
	}
}

void MonsterEditor::handle_edit_string()
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

void MonsterEditor::handle_picker(const Renderer& renderer)
{
	int totalSheets = renderer.get_loaded_sheet_count();

	auto advance_sheet = [&](int dir)
	{
		for (int sheetsTried = 0; sheetsTried < totalSheets; ++sheetsTried)
		{
			pickerSheet = (pickerSheet + dir + totalSheets) % totalSheets;
			if (renderer.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
			{
				break;
			}
		}
		pickerScroll = 0;
	};

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
	{
		advance_sheet(1);
	}

	if (IsKeyPressed(KEY_LEFT))
	{
		advance_sheet(-1);
	}
	if (IsKeyPressed(KEY_RIGHT))
	{
		advance_sheet(1);
	}
	if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F2))
	{
		mode = Mode::NORMAL;
		return;
	}

	int sheetRows = renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	::Vector2 mouse = GetMousePosition();
	bool isMouseInPicker = mouse.x >= LIST_WIDTH;

	if (isMouseInPicker)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			pickerScroll = std::clamp(
				pickerScroll - static_cast<int>(wheel),
				0,
				std::max(0, sheetRows - 1));
		}
	}

	int gridY = HEADER_HEIGHT + PICKER_SUB_HEADER_HEIGHT;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isMouseInPicker)
	{
		const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
		int col = static_cast<int>(mouse.x - LIST_WIDTH - PICKER_PAD) / (pickerTileSize + 2);
		int row = pickerScroll + static_cast<int>(mouse.y - gridY) / (pickerTileSize + 2);
		int sheetColumns = renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));

		if (col >= 0 && col < sheetColumns && row >= 0 && row < sheetRows)
		{
			working.symbol = TileRef{ static_cast<TileSheet>(pickerSheet), col, row };
			mode = Mode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void MonsterEditor::render(const GameContext& ctx) const
{
	assert(ctx.monsterRegistry && "MonsterEditor::render: called without a monsterRegistry");

	const Renderer& renderer = *ctx.renderer;
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(renderer);
	render_list(renderer, *ctx.monsterRegistry);

	if (mode == Mode::TILE_PICKER)
	{
		render_picker(renderer);
	}
	else
	{
		render_fields(renderer);
	}

	render_hint(renderer);
}

void MonsterEditor::render_header(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 20, 20, 40, 255 });
	renderer.draw_text_color(Vector2D{ 8, 6 }, "MONSTER EDITOR", Color{ 255, 255, 180, 255 });
	renderer.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		Color{ 130, 130, 100, 255 });
}

int MonsterEditor::body_height(const Renderer& renderer)
{
	return renderer.get_screen_height() - HEADER_HEIGHT - HINT_HEIGHT;
}

void MonsterEditor::render_list(const Renderer& renderer, const MonsterRegistry& monsters) const
{
	int bodyY = HEADER_HEIGHT;
	int bodyHeight = body_height(renderer);

	DrawRectangle(0, bodyY, LIST_WIDTH, bodyHeight, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_WIDTH, bodyY, LIST_WIDTH, bodyY + bodyHeight, Color{ 80, 80, 120, 255 });

	const size_t visibleCount = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t scroll = listCursor.first_visible(visibleCount);

	::Vector2 mouse = GetMousePosition();

	for (size_t monsterIndex = scroll; monsterIndex < keys.size(); ++monsterIndex)
	{
		int itemY = bodyY + static_cast<int>(monsterIndex - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > bodyY + bodyHeight)
		{
			break;
		}

		// Section separators
		if (monsterIndex > scroll)
		{
			const bool previousIsBuiltin = monsters.is_builtin(keys[monsterIndex - 1]);
			const bool currentIsCustom = !monsters.is_builtin(keys[monsterIndex]);

			if (previousIsBuiltin && currentIsCustom)
			{
				DrawLine(LIST_PAD, itemY, LIST_WIDTH - LIST_PAD, itemY, Color{ 70, 70, 70, 200 });
			}
		}

		bool isSelected = (monsterIndex == listCursor.current_row());
		bool hovered = mouse.x >= 0 && mouse.x < LIST_WIDTH && mouse.y >= itemY && mouse.y < itemY + ITEM_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (isSelected && focus == 0)
		{
			bgColor = Color{ 60, 60, 0, 200 };
		}
		else if (isSelected)
		{
			bgColor = Color{ 40, 40, 0, 150 };
		}
		else if (hovered)
		{
			bgColor = Color{ 30, 30, 30, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		const TileRef tile = isSelected ? working.symbol : monsters.get_tile(keys[monsterIndex]);
		renderer.draw_tile_screen_sized(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 }, tile, LIST_TILE_SIZE);

		const bool isCustom = !monsters.is_builtin(keys[monsterIndex]);

		std::string displayName = isSelected
			? working.name
			: monsters.get_params(keys[monsterIndex]).name;
		if (displayName.empty())
		{
			displayName = prettify_key(keys[monsterIndex]);
		}
		if (isCustom)
		{
			displayName += " *";
		}

		const Color textColor = isSelected ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ LIST_PAD + LIST_TILE_SIZE + 4, itemY + (ITEM_HEIGHT - 16) / 2 }, displayName, textColor);
	}
}

void MonsterEditor::render_fields(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = body_height(renderer);

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 8, 8, 16, 255 });

	const size_t visible = whole_rows_within(fieldsHeight, FIELD_HEIGHT);
	const size_t scroll = fieldCursor.first_visible(visible);

	::Vector2 mouse = GetMousePosition();

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
			bgColor = Color{ 60, 60, 0, 200 };
		}
		else if (isSelected)
		{
			bgColor = Color{ 40, 40, 0, 140 };
		}
		else if (hovered)
		{
			bgColor = Color{ 20, 20, 10, 120 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(panelX, itemY, fieldsWidth, FIELD_HEIGHT, bgColor);
		}

		Color labelColor = isSelected ? Color{ 255, 220, 80, 255 } : Color{ 150, 150, 150, 255 };
		Color valueColor = isSelected ? Color{ 255, 255, 200, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fieldId), labelColor);

		if (fieldId == FieldId::TILE)
		{
			renderer.draw_tile_screen_sized(Vector2D{ screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2 }, working.symbol, LIST_TILE_SIZE);
			if (isSelected)
			{
				DrawRectangleLines(screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2, LIST_TILE_SIZE, LIST_TILE_SIZE, Color{ 255, 255, 0, 255 });
			}
		}
		else
		{
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
}

void MonsterEditor::render_picker(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int panelX = LIST_WIDTH;
	int panelWidth = screenWidth - LIST_WIDTH;
	int bodyY = HEADER_HEIGHT;
	int bodyHeight = body_height(renderer);

	DrawRectangle(panelX, bodyY, panelWidth, bodyHeight, Color{ 8, 8, 24, 255 });

	DrawRectangle(panelX, bodyY, panelWidth, PICKER_SUB_HEADER_HEIGHT, Color{ 15, 15, 40, 255 });
	int totalSheets = renderer.get_loaded_sheet_count();
	std::string header = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		renderer.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		totalSheets);
	renderer.draw_text_color(Vector2D{ panelX + PICKER_PAD, bodyY + 6 }, header, Color{ 200, 200, 120, 255 });

	int gridY = bodyY + PICKER_SUB_HEADER_HEIGHT;
	int gridHeight = bodyHeight - PICKER_SUB_HEADER_HEIGHT;
	int sheetColumns = renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));
	int sheetRows = renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const int pickerTileSize = picker_tile_size(renderer, pickerSheet);

	::Vector2 mouse = GetMousePosition();

	BeginScissorMode(panelX, gridY, panelWidth, gridHeight);

	for (int row = pickerScroll; row < sheetRows; ++row)
	{
		int cellPixelY = gridY + (row - pickerScroll) * (pickerTileSize + 2);
		if (cellPixelY >= gridY + gridHeight)
		{
			break;
		}

		for (int col = 0; col < sheetColumns; ++col)
		{
			int cellPixelX = panelX + PICKER_PAD + col * (pickerTileSize + 2);
			TileRef tile{ static_cast<TileSheet>(pickerSheet), col, row };

			bool isCurrentTile = (tile == working.symbol);
			bool hovered = mouse.x >= cellPixelX && mouse.x < cellPixelX + pickerTileSize && mouse.y >= cellPixelY && mouse.y < cellPixelY + pickerTileSize;

			if (isCurrentTile)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 60, 60, 0, 220 });
			}
			else if (hovered)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 40, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tile, pickerTileSize);

			if (isCurrentTile)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 255, 255, 0, 255 });
			}
		}
	}

	EndScissorMode();
}

void MonsterEditor::render_hint(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int hintY = screenHeight - HINT_HEIGHT;

	DrawRectangle(0, hintY, screenWidth, HINT_HEIGHT, Color{ 20, 20, 40, 255 });

	std::string hintText;
	bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;

	if (isSaveRecent)
	{
		hintText = "Saved!";
	}
	else if (mode == Mode::EDIT_STRING)
	{
		hintText = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (mode == Mode::TILE_PICKER)
	{
		hintText = "Tile Picker  --  Left/Right:sheet  Click:assign  Esc/F2:back";
	}
	else if (focus == 0)
	{
		hintText = "[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit";
	}
	else
	{
		hintText = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hintColor = isSaveRecent ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	renderer.draw_text_color(Vector2D{ 8, hintY + 6 }, hintText, hintColor);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& MonsterEditor::current_key() const
{
	if (keys.empty())
	{
		throw std::out_of_range("MonsterEditor::current_key -- key list is empty");
	}
	return keys[listCursor.current_row()];
}

MonsterEditor::FieldId MonsterEditor::current_field() const
{
	return static_cast<FieldId>(fieldCursor.current_row());
}

std::string MonsterEditor::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

void MonsterEditor::set_damage_dice(int num, int sides, int bonus)
{
	working.damage = DamageInfo{ DiceExpr{ num, sides, bonus }, working.damage.damageType };
}

std::string MonsterEditor::field_value(FieldId field) const
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		return plain_text(*descriptor.plain, working);
	}

	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::CORPSE:
	{
		return working.corpseName;
	}
	case FieldId::CORPSE_TASTE:
	{
		return working.corpseTaste;
	}
	case FieldId::HD_LOWEST:
	{
		return std::format("{}", working.hitDice.counts.front().count);
	}
	case FieldId::HD_HIGHEST:
	{
		return std::format("{}", highest_count(working.hitDice).count);
	}
	case FieldId::HD_SIDES:
	{
		return std::format("{}", working.hitDice.sides);
	}
	case FieldId::HD_BONUS:
	{
		return std::format("{}", working.hitDice.bonus);
	}
	case FieldId::XP_COUNT:
	{
		return std::format("{} HD", working.hitDice.counts.at(selectedXpCount).count);
	}
	case FieldId::XP:
	{
		return std::format("{}", working.hitDice.counts.at(selectedXpCount).xp);
	}
	case FieldId::STR_SOURCE:
	{
		return working.strength.has_value() ? "Printed in its entry" : "Table 6 by size";
	}
	case FieldId::STR_SCORE:
	{
		return working.strength.has_value() ? std::format("{}", working.strength->score) : "-";
	}
	case FieldId::STR_EXCEPTIONAL:
	{
		// 0 is no percentile, 1 to 100 is the percentile with 100 as 18/00, and only an 18 has one.
		if (!working.strength.has_value())
		{
			return "-";
		}
		const MonsterStrength strength = *working.strength;
		if (strength.score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			return "only on an 18";
		}
		if (strength.exceptional == MONSTER_STRENGTH_EXCEPTIONAL_NONE)
		{
			return "0 (none)";
		}
		if (strength.exceptional == MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM)
		{
			return std::format("{} (18/00)", strength.exceptional);
		}
		return std::format("{} (18/{:02})", strength.exceptional, strength.exceptional);
	}
	case FieldId::WEAPON:
	{
		return working.naturalAttack;
	}
	case FieldId::DMG_NUM:
	{
		return std::format("{}", working.damage.dice.num);
	}
	case FieldId::DMG_SIDES:
	{
		return std::format("{}", working.damage.dice.sides);
	}
	case FieldId::DMG_BONUS:
	{
		return std::format("{}", working.damage.dice.bonus);
	}
	case FieldId::DMG_TYPE:
	{
		return std::string(damage_type_name(working.damage.damageType));
	}
	case FieldId::AI_TYPE:
	{
		return std::string(encode_ai_type(working.aiType));
	}
	case FieldId::ETHICS:
	{
		return std::string(encode_ethics(working.ethics));
	}
	case FieldId::MORALITY:
	{
		return std::string(encode_morality(working.morality));
	}
	case FieldId::SIZE:
	{
		return std::string(encode_creature_size(working.size));
	}
	case FieldId::INTELLIGENCE:
	{
		return std::string(encode_intelligence_rating(working.intelligence));
	}
	case FieldId::CAN_SWIM:
	{
		return working.canSwim ? "yes" : "no";
	}
	case FieldId::CAN_FLY:
	{
		return working.canFly ? "yes" : "no";
	}
	case FieldId::WALKS_WEBS:
	{
		return working.walksWebs ? "yes" : "no";
	}
	case FieldId::ATTITUDE:
	{
		return std::string(encode_attitude(working.attitude));
	}
	case FieldId::DISPLACEABLE:
	{
		return working.displaceable ? "yes" : "no";
	}
	case FieldId::CASTER_CLASS:
	{
		return working.spellcasterTraining.has_value() ? std::string(encode_caster_class(working.spellcasterTraining->casterClass)) : "none";
	}
	case FieldId::CASTER_LEVEL:
	{
		return working.spellcasterTraining.has_value() ? std::format("{}", working.spellcasterTraining->casterLevel) : "-";
	}
	case FieldId::INNATE_POWER:
	{
		if (working.innatePowers.empty())
		{
			return "none";
		}
		return std::format("{} of {}", selectedPower + 1, working.innatePowers.size());
	}
	case FieldId::INNATE_SPELL:
	{
		return working.innatePowers.empty() ? "" : working.innatePowers.at(selectedPower).spell;
	}
	case FieldId::INNATE_FREQUENCY:
	{
		return working.innatePowers.empty() ? "-" : std::string(encode_power_frequency(working.innatePowers.at(selectedPower).frequency));
	}
	case FieldId::CORPSE_EDIBLE:
	{
		return is_corpse_edible(working.corpseEdibility) ? "yes" : "no";
	}
	case FieldId::TILE:
	{
		return "(tile)";
	}
	default:
	{
		refuse_misrouted_field(descriptor.label, "MonsterEditor::field_value");
	}
	}
}

void MonsterEditor::field_adjust(FieldId field, int delta)
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		adjust_plain(*descriptor.plain, working, delta);
		return;
	}

	auto clamp_val = [](int value, int step, int low, int high) -> int
	{
		return std::clamp(value + step, low, high);
	};

	switch (field)
	{
	case FieldId::HD_LOWEST:
	{
		working.hitDice = with_lowest_count(working.hitDice, working.hitDice.counts.front().count + delta);
		// The range may have shrunk under the selected count.
		selectedXpCount = std::min(selectedXpCount, working.hitDice.counts.size() - 1);
		assert(is_valid_monster_hit_dice(working.hitDice) && "MonsterEditor: the lowest count left hit dice the loader refuses");
		break;
	}
	case FieldId::HD_HIGHEST:
	{
		working.hitDice = with_highest_count(working.hitDice, highest_count(working.hitDice).count + delta);
		selectedXpCount = std::min(selectedXpCount, working.hitDice.counts.size() - 1);
		assert(is_valid_monster_hit_dice(working.hitDice) && "MonsterEditor: the highest count left hit dice the loader refuses");
		break;
	}
	case FieldId::HD_SIDES:
	{
		working.hitDice.sides = clamp_val(working.hitDice.sides, delta, 1, 20);
		break;
	}
	case FieldId::HD_BONUS:
	{
		working.hitDice.bonus = clamp_val(working.hitDice.bonus, delta, -99, 99);
		break;
	}
	case FieldId::XP_COUNT:
	{
		// One count a step, whatever the step size, and never past either end.
		if (delta < 0 && selectedXpCount > 0)
		{
			--selectedXpCount;
		}
		else if (delta > 0 && selectedXpCount + 1 < working.hitDice.counts.size())
		{
			++selectedXpCount;
		}
		break;
	}
	case FieldId::XP:
	{
		HitDiceCount& selected = working.hitDice.counts.at(selectedXpCount);
		selected.xp = clamp_val(selected.xp, delta, 0, 99999);
		break;
	}
	case FieldId::STR_SCORE:
	{
		// A Table 6 monster has no score to adjust.
		if (!working.strength.has_value())
		{
			break;
		}
		MonsterStrength& strength = *working.strength;
		strength.score = clamp_val(strength.score, delta, MONSTER_STRENGTH_SCORE_MINIMUM, MONSTER_STRENGTH_SCORE_MAXIMUM);
		// A percentile rides only on an 18, so leaving 18 drops it rather than writing what the loader refuses.
		if (strength.score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			strength.exceptional = MONSTER_STRENGTH_EXCEPTIONAL_NONE;
		}
		assert(is_valid_monster_strength(strength) && "MonsterEditor: the Strength score left a Strength the loader refuses");
		break;
	}
	case FieldId::STR_EXCEPTIONAL:
	{
		if (!working.strength.has_value() || working.strength->score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			break;
		}
		MonsterStrength& strength = *working.strength;
		strength.exceptional = clamp_val(strength.exceptional, delta, MONSTER_STRENGTH_EXCEPTIONAL_NONE, MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM);
		assert(is_valid_monster_strength(strength) && "MonsterEditor: the percentile left a Strength the loader refuses");
		break;
	}
	// The dice are the record: every change rebuilds the damage, so its text and
	// its range follow rather than being edited into disagreement.
	case FieldId::DMG_NUM:
	{
		set_damage_dice(clamp_val(working.damage.dice.num, delta, 1, 99), working.damage.dice.sides, working.damage.dice.bonus);
		break;
	}
	case FieldId::DMG_SIDES:
	{
		set_damage_dice(working.damage.dice.num, clamp_val(working.damage.dice.sides, delta, 2, 20), working.damage.dice.bonus);
		break;
	}
	case FieldId::DMG_BONUS:
	{
		set_damage_dice(working.damage.dice.num, working.damage.dice.sides, clamp_val(working.damage.dice.bonus, delta, -99, 99));
		break;
	}
	case FieldId::CASTER_LEVEL:
	{
		// A monster that memorizes nothing has no level to adjust; the tables print 1st to 20th.
		if (working.spellcasterTraining.has_value())
		{
			working.spellcasterTraining->casterLevel = clamp_val(working.spellcasterTraining->casterLevel, delta, 1, 20);
		}
		break;
	}
	case FieldId::INNATE_POWER:
	{
		// One power a step, as the XP field's count is chosen, and never past either end.
		if (delta < 0 && selectedPower > 0)
		{
			--selectedPower;
		}
		else if (delta > 0 && selectedPower + 1 < working.innatePowers.size())
		{
			++selectedPower;
		}
		break;
	}
	default:
	{
		refuse_misrouted_field(descriptor.label, "MonsterEditor::field_adjust");
	}
	}
}

void MonsterEditor::field_toggle(FieldId field)
{
	if (field == FieldId::AI_TYPE)
	{
		working.aiType = next_authorable_ai_type(working.aiType);
	}
	else if (field == FieldId::CAN_SWIM)
	{
		working.canSwim = !working.canSwim;
	}
	else if (field == FieldId::CAN_FLY)
	{
		working.canFly = !working.canFly;
	}
	else if (field == FieldId::WALKS_WEBS)
	{
		working.walksWebs = !working.walksWebs;
	}
	else if (field == FieldId::ATTITUDE)
	{
		working.attitude = next_attitude(working.attitude);
	}
	else if (field == FieldId::DISPLACEABLE)
	{
		working.displaceable = !working.displaceable;
	}
	else if (field == FieldId::CORPSE_EDIBLE)
	{
		working.corpseEdibility = is_corpse_edible(working.corpseEdibility) ? CorpseEdibility::INEDIBLE : CorpseEdibility::EDIBLE;
	}
	else if (field == FieldId::ETHICS)
	{
		working.ethics = next_ethics(working.ethics);
	}
	else if (field == FieldId::MORALITY)
	{
		working.morality = next_morality(working.morality);
	}
	else if (field == FieldId::SIZE)
	{
		working.size = next_creature_size(working.size);
	}
	else if (field == FieldId::INTELLIGENCE)
	{
		working.intelligence = next_intelligence_rating(working.intelligence);
	}
	else if (field == FieldId::STR_SOURCE)
	{
		// A printed Strength starts at 10, the middle of 3d6, for the author to set to the entry's figure.
		constexpr MonsterStrength startingPrintedStrength{ 10, MONSTER_STRENGTH_EXCEPTIONAL_NONE };
		if (working.strength.has_value())
		{
			working.strength.reset();
		}
		else
		{
			working.strength = startingPrintedStrength;
		}
	}
	else if (field == FieldId::DMG_TYPE)
	{
		// The dice are unchanged; only what they deal is.
		working.damage = DamageInfo{ working.damage.dice, next_damage_type(working.damage.damageType) };
	}
	else if (field == FieldId::CASTER_CLASS)
	{
		// None, then a 1st-level wizard, then a priest at the same level, then none again.
		if (!working.spellcasterTraining.has_value())
		{
			working.spellcasterTraining = SpellcasterTraining{ CasterClass::WIZARD, 1 };
		}
		else if (working.spellcasterTraining->casterClass == CasterClass::WIZARD)
		{
			working.spellcasterTraining->casterClass = CasterClass::CLERIC;
		}
		else
		{
			working.spellcasterTraining.reset();
		}
	}
	else if (field == FieldId::INNATE_FREQUENCY)
	{
		// With no power listed there is no frequency to step.
		if (!working.innatePowers.empty())
		{
			InnatePower& power = working.innatePowers.at(selectedPower);
			if (power.frequency == PowerFrequency::AT_WILL)
			{
				power.frequency = PowerFrequency::ONCE_PER_DAY;
			}
			else
			{
				power.frequency = PowerFrequency::AT_WILL;
			}
		}
	}
	else
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"MonsterEditor::field_toggle");
	}
}

void MonsterEditor::field_set_string(FieldId field, std::string newValue)
{
	switch (field)
	{
	case FieldId::NAME:
	{
		working.name = std::move(newValue);
		break;
	}
	case FieldId::CORPSE:
	{
		working.corpseName = std::move(newValue);
		break;
	}
	case FieldId::CORPSE_TASTE:
	{
		working.corpseTaste = std::move(newValue);
		break;
	}
	case FieldId::WEAPON:
	{
		working.naturalAttack = std::move(newValue);
		break;
	}
	case FieldId::INNATE_SPELL:
	{
		if (working.innatePowers.empty())
		{
			if (!newValue.empty())
			{
				working.innatePowers.push_back(InnatePower{ std::move(newValue), PowerFrequency::AT_WILL, 0 });
				selectedPower = 0;
			}
		}
		else if (newValue.empty())
		{
			working.innatePowers.erase(working.innatePowers.begin() + static_cast<std::ptrdiff_t>(selectedPower));
			selectedPower = selectedPower > 0 ? selectedPower - 1 : 0;
		}
		else
		{
			working.innatePowers.at(selectedPower).spell = std::move(newValue);
		}
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"MonsterEditor::field_set_string");
	}
	}
}

// end of file: MonsterEditor.cpp
