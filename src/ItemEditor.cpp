// file: ItemEditor.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "BuffType.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "ItemClassification.h"
#include "ItemEditor.h"
#include "ItemRegistry.h"
#include "MagicalItemEffects.h"
#include "Menu.h"
#include "Paths.h"
#include "Renderer.h"
#include "TargetMode.h"
#include "Weapons.h"

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

static_assert(is_well_formed_table(ItemEditor::FIELDS), "ItemEditor::FIELDS lists a field out of FieldId order, or a plain number on a field that is not ADJUST");

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

std::string prettify_key(std::string_view key)
{
	std::string result;
	bool isWordStart = true;
	for (char character : key)
	{
		if (character == '_')
		{
			result += ' ';
			isWordStart = true;
		}
		else if (isWordStart)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			isWordStart = false;
		}
		else
		{
			result += character;
		}
	}
	return result;
}

std::string_view consumable_effect_str(ConsumableEffect effect)
{
	switch (effect)
	{
	case ConsumableEffect::NONE:
	{
		return "none";
	}
	case ConsumableEffect::HEAL:
	{
		return "heal";
	}
	case ConsumableEffect::ADD_BUFF:
	{
		return "add_buff";
	}
	case ConsumableEffect::FAIL:
	{
		return "fail";
	}
	}
	return "none";
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ItemEditor::enter(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::enter: called without an itemRegistry");

	contentRegistry = ctx.contentRegistry;
	active = true;
	mode = Mode::NORMAL;
	focus = 0;

	keys = ctx.itemRegistry->get_all_keys();

	listCursor.reset(keys.size());
	fieldCursor.reset(FIELD_COUNT);
	pickerSheet = 0;
	pickerScroll = 0;

	load_working(*ctx.itemRegistry);
}

void ItemEditor::exit(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::exit: called without an itemRegistry");

	commit_working(*ctx.itemRegistry);
	active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void ItemEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	render(ctx);
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void ItemEditor::load_working(const ItemRegistry& items)
{
	if (keys.empty())
	{
		return;
	}

	const std::string& key = current_key();
	const ItemParams& params = items.get_params(key);
	working = params;
	workingTile = contentRegistry ? contentRegistry->get_tile(key) : TileRef{};
	fieldCursor.reset(FIELD_COUNT);
	mode = Mode::NORMAL;
	editBuffer.clear();
}

void ItemEditor::commit_working(ItemRegistry& items)
{
	if (keys.empty())
	{
		return;
	}
	const std::string& key = current_key();
	items.set_name_category(key, working.name, working.category);
	items.set_params(key, working);
	if (contentRegistry)
	{
		contentRegistry->set_tile(key, workingTile);
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ItemEditor::handle_input(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::handle_input: called without an itemRegistry");

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (ctrl && IsKeyPressed(KEY_S))
	{
		commit_working(*ctx.itemRegistry);
		ctx.itemRegistry->save(Paths::ITEMS);
		if (contentRegistry)
		{
			ContentRegistryIO::save(*contentRegistry, Paths::CONTENT_TILES);
		}
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

void ItemEditor::handle_normal(const GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::handle_normal: called without an itemRegistry");

	ItemRegistry& items = *ctx.itemRegistry;
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	const Renderer& renderer = *ctx.renderer;
	int bodyHeight = body_height(renderer);
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
				commit_working(items);
				listCursor.select(*row);
				load_working(items);
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
			commit_working(items);
			listCursor.move_up();
			load_working(items);
		}
		// A screen too short for one row has nowhere to scroll the cursor to.
		else if (IsKeyPressed(KEY_DOWN) && listCursor.can_move_down() && visibleListRows > 0)
		{
			commit_working(items);
			listCursor.move_down(visibleListRows);
			load_working(items);
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working(items);
			ItemParams defaults;
			defaults.itemClass = ItemClass::SWORD;
			defaults.pickableType = PickableType::WEAPON;
			defaults.value = 10;
			defaults.baseWeight = 10;
			defaults.levelMin = 1;
			defaults.levelMax = 5;
			defaults.handRequirement = HandRequirement::ONE_HANDED;
			defaults.weaponSize = WeaponSize::MEDIUM;
			const std::string newKey = items.add_custom("New Item", "weapon", defaults);
			keys = items.get_all_keys();
			listCursor.set_row_count(keys.size());
			for (size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex)
			{
				if (keys[keyIndex] == newKey)
				{
					listCursor.select(keyIndex);
					break;
				}
			}
			load_working(items);
			focus = 1;
		}
		else if (IsKeyPressed(KEY_DELETE))
		{
			if (!items.is_builtin_key(current_key()))
			{
				// An item deleted before any commit was never given a sprite.
				if (contentRegistry && contentRegistry->all_tiles().contains(current_key()))
				{
					contentRegistry->remove_tile(current_key());
				}
				items.remove_custom(current_key());
				keys = items.get_all_keys();
				listCursor.set_row_count(keys.size());
				load_working(items);
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

	FieldId fieldId = current_field();

	const FieldKind kind = descriptor_of(FIELDS, fieldId).kind;

	if (IsKeyPressed(KEY_F2) || (kind == FieldKind::TILE && IsKeyPressed(KEY_ENTER)))
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

void ItemEditor::handle_edit_string()
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

void ItemEditor::handle_picker(const Renderer& renderer)
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
			workingTile = TileRef{ static_cast<TileSheet>(pickerSheet), col, row };
			mode = Mode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void ItemEditor::render(const GameContext& ctx) const
{
	assert(ctx.itemRegistry && "ItemEditor::render: called without an itemRegistry");

	const Renderer& renderer = *ctx.renderer;
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(renderer);
	render_list(renderer, *ctx.itemRegistry);

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

void ItemEditor::render_header(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 0, 20, 40, 255 });
	renderer.draw_text_color(Vector2D{ 8, 6 }, "ITEM EDITOR", Color{ 180, 255, 180, 255 });
	renderer.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		Color{ 100, 130, 100, 255 });
}

int ItemEditor::body_height(const Renderer& renderer)
{
	return renderer.get_screen_height() - HEADER_HEIGHT - HINT_HEIGHT;
}

void ItemEditor::render_list(const Renderer& renderer, const ItemRegistry& items) const
{
	int bodyY = HEADER_HEIGHT;
	int bodyHeight = body_height(renderer);

	DrawRectangle(0, bodyY, LIST_WIDTH, bodyHeight, Color{ 5, 10, 5, 255 });
	DrawLine(LIST_WIDTH, bodyY, LIST_WIDTH, bodyY + bodyHeight, Color{ 80, 120, 80, 255 });

	const size_t visibleCount = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t scroll = listCursor.first_visible(visibleCount);

	::Vector2 mouse = GetMousePosition();

	for (size_t itemIndex = scroll; itemIndex < keys.size(); ++itemIndex)
	{
		int itemY = bodyY + static_cast<int>(itemIndex - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > bodyY + bodyHeight)
		{
			break;
		}

		bool isSelected = (itemIndex == listCursor.current_row());
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
			bgColor = Color{ 20, 30, 20, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		TileRef tile = isSelected
			? workingTile
			: (contentRegistry ? contentRegistry->get_tile(keys[itemIndex]) : TileRef{});
		renderer.draw_tile_screen_sized(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 }, tile, LIST_TILE_SIZE);

		const bool isCustom = !items.is_builtin_key(keys[itemIndex]);

		std::string displayName = isSelected
			? working.name
			: std::string{ items.get_params(keys[itemIndex]).name };

		if (displayName.empty())
		{
			displayName = prettify_key(keys[itemIndex]);
		}

		if (isCustom)
		{
			displayName += " *";
		}

		Color textColor = isSelected
			? Color{ 150, 255, 150, 255 }
			: Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ LIST_PAD + LIST_TILE_SIZE + 4, itemY + (ITEM_HEIGHT - 16) / 2 }, displayName, textColor);
	}
}

void ItemEditor::render_fields(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = body_height(renderer);

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 5, 8, 5, 255 });

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
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (isSelected)
		{
			bgColor = Color{ 0, 40, 0, 140 };
		}
		else if (hovered)
		{
			bgColor = Color{ 10, 20, 10, 120 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(panelX, itemY, fieldsWidth, FIELD_HEIGHT, bgColor);
		}

		Color labelColor = isSelected ? Color{ 150, 255, 150, 255 } : Color{ 150, 150, 150, 255 };
		Color valueColor = isSelected ? Color{ 220, 255, 220, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fieldId), labelColor);

		if (fieldId == FieldId::TILE)
		{
			renderer.draw_tile_screen_sized(Vector2D{ screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2 }, workingTile, LIST_TILE_SIZE);
			if (isSelected)
			{
				DrawRectangleLines(
					screenWidth - LIST_TILE_SIZE - 12,
					itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2,
					LIST_TILE_SIZE,
					LIST_TILE_SIZE,
					Color{ 0, 255, 100, 255 });
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

void ItemEditor::render_picker(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int panelX = LIST_WIDTH;
	int panelWidth = screenWidth - LIST_WIDTH;
	int bodyY = HEADER_HEIGHT;
	int bodyHeight = body_height(renderer);

	DrawRectangle(panelX, bodyY, panelWidth, bodyHeight, Color{ 5, 8, 24, 255 });

	DrawRectangle(panelX, bodyY, panelWidth, PICKER_SUB_HEADER_HEIGHT, Color{ 8, 15, 40, 255 });
	int totalSheets = renderer.get_loaded_sheet_count();
	std::string header = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		renderer.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		totalSheets);
	renderer.draw_text_color(Vector2D{ panelX + PICKER_PAD, bodyY + 6 }, header, Color{ 120, 200, 200, 255 });

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

			bool isCurrentTile = (tile == workingTile);
			bool hovered = mouse.x >= cellPixelX && mouse.x < cellPixelX + pickerTileSize && mouse.y >= cellPixelY && mouse.y < cellPixelY + pickerTileSize;

			if (isCurrentTile)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 0, 60, 0, 220 });
			}
			else if (hovered)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 20, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tile, pickerTileSize);

			if (isCurrentTile)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 0, 255, 100, 255 });
			}
		}
	}

	EndScissorMode();
}

void ItemEditor::render_hint(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int hintY = screenHeight - HINT_HEIGHT;

	DrawRectangle(0, hintY, screenWidth, HINT_HEIGHT, Color{ 0, 20, 40, 255 });

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
		hintText = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/toggle  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hintColor = isSaveRecent ? Color{ 100, 255, 100, 255 } : Color{ 120, 160, 120, 255 };
	renderer.draw_text_color(Vector2D{ 8, hintY + 6 }, hintText, hintColor);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& ItemEditor::current_key() const
{
	if (keys.empty())
	{
		throw std::out_of_range("ItemEditor::current_key -- key list is empty");
	}
	return keys[listCursor.current_row()];
}

ItemEditor::FieldId ItemEditor::current_field() const
{
	return static_cast<FieldId>(fieldCursor.current_row());
}

std::string ItemEditor::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

std::string ItemEditor::field_value(FieldId field) const
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		return plain_text(*descriptor.plain, working);
	}

	const ItemParams& params = working;
	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::CATEGORY:
	{
		return working.category;
	}
	case FieldId::ITEM_CLASS:
	{
		return std::string{ encode_item_class(params.itemClass) };
	}
	case FieldId::PICKABLE_TYPE:
	{
		return std::string{ encode_pickable_type(params.pickableType) };
	}
	case FieldId::COLOR:
	{
		return std::string{ color_pair_name(params.color) };
	}
	case FieldId::LEVEL_SCALING:
	{
		return std::format("{:.2f}", params.levelScaling);
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		return std::string{ consumable_effect_str(params.consumableEffect) };
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		return std::string{ encode_buff_type(params.consumableBuffType) };
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		return std::format("{} ({})", params.consumableAmount.num, to_text(params.consumableAmount));
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		return std::format("{}", params.consumableAmount.sides);
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		return std::format("{}", params.consumableAmount.bonus);
	}
	case FieldId::DURATION_DICE:
	{
		return std::format("{} ({})", params.duration.num, to_text(params.duration));
	}
	case FieldId::DURATION_SIDES:
	{
		return std::format("{}", params.duration.sides);
	}
	case FieldId::DURATION_BONUS:
	{
		return std::format("{}", params.duration.bonus);
	}
	case FieldId::DURATION_UNIT:
	{
		return std::string{ encode_duration_unit(params.durationUnit) };
	}
	case FieldId::TARGET_MODE:
	{
		return std::string{ encode_target_mode(params.targetMode) };
	}
	case FieldId::SCROLL_ANIM:
	{
		return std::string{ encode_scroll_animation(params.scrollAnimation) };
	}
	case FieldId::RANGED:
	{
		return params.ranged ? "yes" : "no";
	}
	case FieldId::HAND_REQUIREMENT:
	{
		return std::string{ encode_hand_requirement(params.handRequirement) };
	}
	case FieldId::WEAPON_SIZE:
	{
		return std::string{ encode_weapon_size(params.weaponSize) };
	}
	case FieldId::EFFECT:
	{
		return std::string{ encode_magical_effect(params.effect) };
	}
	case FieldId::APPLICATION:
	{
		return std::string{ encode_buff_application(params.application) };
	}
	case FieldId::TILE:
	{
		return "(tile)";
	}
	default:
	{
		refuse_misrouted_field(descriptor.label, "ItemEditor::field_value");
	}
	}
}

void ItemEditor::field_adjust(FieldId field, int delta)
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

	ItemParams& params = working;
	switch (field)
	{
	case FieldId::LEVEL_SCALING:
	{
		float raw = params.levelScaling + static_cast<float>(delta) * 0.01f;
		params.levelScaling = std::clamp(raw, 0.0f, 5.0f);
		break;
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		// No dice is a fixed amount; any dice need a die to roll, and a d2 is the least.
		params.consumableAmount.num = clamp_val(params.consumableAmount.num, delta, 0, 99);
		params.consumableAmount.sides = params.consumableAmount.num == 0 ? 0 : std::max(params.consumableAmount.sides, 2);
		break;
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		params.consumableAmount.sides = params.consumableAmount.num == 0 ? 0 : clamp_val(params.consumableAmount.sides, delta, 2, 20);
		break;
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		params.consumableAmount.bonus = clamp_val(params.consumableAmount.bonus, delta, 0, 9999);
		break;
	}
	case FieldId::DURATION_DICE:
	{
		// No dice is a fixed duration; any dice need a die to roll, and a d2 is the least.
		params.duration.num = clamp_val(params.duration.num, delta, 0, 99);
		params.duration.sides = params.duration.num == 0 ? 0 : std::max(params.duration.sides, 2);
		break;
	}
	case FieldId::DURATION_SIDES:
	{
		params.duration.sides = params.duration.num == 0 ? 0 : clamp_val(params.duration.sides, delta, 2, 20);
		break;
	}
	case FieldId::DURATION_BONUS:
	{
		params.duration.bonus = clamp_val(params.duration.bonus, delta, 0, 9999);
		break;
	}
	default:
	{
		refuse_misrouted_field(descriptor.label, "ItemEditor::field_adjust");
	}
	}
}

void ItemEditor::field_toggle(FieldId field)
{
	ItemParams& params = working;
	switch (field)
	{
	case FieldId::COLOR:
	{
		params.color = next_color_pair(params.color);
		break;
	}
	case FieldId::ITEM_CLASS:
	{
		params.itemClass = next_item_class(params.itemClass);
		break;
	}
	case FieldId::PICKABLE_TYPE:
	{
		params.pickableType = next_pickable_type(params.pickableType);
		break;
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		params.consumableEffect = next_consumable_effect(params.consumableEffect);
		break;
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		params.consumableBuffType = next_buff_type(params.consumableBuffType);
		break;
	}
	case FieldId::DURATION_UNIT:
	{
		params.durationUnit = next_duration_unit(params.durationUnit);
		break;
	}
	case FieldId::TARGET_MODE:
	{
		params.targetMode = next_target_mode(params.targetMode);
		break;
	}
	case FieldId::SCROLL_ANIM:
	{
		params.scrollAnimation = next_scroll_animation(params.scrollAnimation);
		break;
	}
	case FieldId::RANGED:
	{
		params.ranged = !params.ranged;
		break;
	}
	case FieldId::HAND_REQUIREMENT:
	{
		params.handRequirement = next_hand_requirement(params.handRequirement);
		break;
	}
	case FieldId::WEAPON_SIZE:
	{
		params.weaponSize = next_weapon_size(params.weaponSize);
		break;
	}
	case FieldId::EFFECT:
	{
		params.effect = next_magical_effect(params.effect);
		break;
	}
	case FieldId::APPLICATION:
	{
		params.application = params.application == BuffApplication::ADD ? BuffApplication::SET : BuffApplication::ADD;
		break;
	}
	default:
	{
		refuse_misrouted_field(descriptor_of(FIELDS, field).label, "ItemEditor::field_toggle");
	}
	}
}

void ItemEditor::field_set_string(FieldId field, std::string newValue)
{
	switch (field)
	{
	case FieldId::NAME:
	{
		working.name = std::move(newValue);
		break;
	}
	case FieldId::CATEGORY:
	{
		working.category = std::move(newValue);
		break;
	}
	default:
	{
		refuse_misrouted_field(descriptor_of(FIELDS, field).label, "ItemEditor::field_set_string");
	}
	}
}

// end of file: ItemEditor.cpp
