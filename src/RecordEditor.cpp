// file: RecordEditor.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>

#include <raylib.h>

#include "EditorField.h"
#include "EditorInput.h"
#include "GameContext.h"
#include "ItemEditor.h"
#include "Menu.h"
#include "RecordEditor.h"
#include "Renderer.h"
#include "TileRef.h"

using namespace EditorLayout;

namespace
{
// The line drawn where the shipped records end and the custom ones begin.
constexpr Color CUSTOM_DIVIDER{ 70, 70, 70, 200 };

// Largest integer multiple of the sheet's own cell size that fits the target,
// so a 16-pixel tile draws at 4x and a 64-pixel tile at 1x - never blurred.
//
// Example:
//   picker_tile_size(renderer, 0);   // a 16-pixel sheet -> 64
int picker_tile_size(const Renderer& renderer, int sheetIndex)
{
	const int cellSize =
		renderer.get_sheet_cell_size(static_cast<TileSheet>(sheetIndex));
	return cellSize * std::max(1, PICKER_TILE_TARGET / cellSize);
}

// The panel Tab moves focus to.
//
// Example:
//   other_panel(EditorFocus::LIST);   // -> EditorFocus::FIELDS
EditorFocus other_panel(EditorFocus current)
{
	if (current == EditorFocus::LIST)
	{
		return EditorFocus::FIELDS;
	}
	return EditorFocus::LIST;
}
} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

template <typename Model>
void RecordEditor<Model>::enter(GameContext& ctx)
{
	active = true;
	mode = EditorMode::NORMAL;
	focus = EditorFocus::LIST;

	keys = model.keys(ctx);

	listCursor.reset(keys.size());
	fieldCursor.reset(Model::FIELD_COUNT);
	pickerSheet = 0;
	pickerScroll = 0;

	load_working(ctx);
}

template <typename Model>
void RecordEditor<Model>::exit(GameContext& ctx)
{
	assert(ctx.menus && "RecordEditor::exit: called without a menu stack");

	commit_working(ctx);
	active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

template <typename Model>
void RecordEditor<Model>::tick(GameContext& ctx)
{
	assert(ctx.renderer && "RecordEditor::tick: called without a renderer");

	// One snapshot serves the frame's input and its drawing.
	const EditorInput input = read_editor_input(*ctx.renderer);
	handle_input(input, ctx);

	render(input, ctx);
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

template <typename Model>
void RecordEditor<Model>::load_working(const GameContext& ctx)
{
	if (keys.empty())
	{
		return;
	}

	model.load(ctx, current_key());
	fieldCursor.reset(Model::FIELD_COUNT);
	mode = EditorMode::NORMAL;
	editBuffer.clear();
}

template <typename Model>
void RecordEditor<Model>::commit_working(GameContext& ctx)
{
	if (keys.empty())
	{
		return;
	}
	model.commit(ctx, current_key());
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

template <typename Model>
void RecordEditor<Model>::handle_input(
	const EditorInput& input,
	GameContext& ctx)
{
	if (input.is_pressed(EditorKey::ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (input.is_held(EditorKey::CONTROL) && input.is_pressed(EditorKey::SAVE))
	{
		commit_working(ctx);
		model.save(ctx);
		lastSaveTime = GetTime();
		return;
	}

	if (input.is_pressed(EditorKey::TAB))
	{
		focus = other_panel(focus);
		return;
	}

	if (mode == EditorMode::NORMAL)
	{
		handle_normal(input, ctx);
	}
	else if (mode == EditorMode::EDIT_STRING)
	{
		handle_edit_string(input);
	}
	else if (mode == EditorMode::TILE_PICKER)
	{
		assert(
			ctx.renderer &&
			"RecordEditor::handle_input: the tile picker needs a renderer");
		handle_picker(input, *ctx.renderer);
	}
}

template <typename Model>
void RecordEditor<Model>::handle_normal(
	const EditorInput& input,
	GameContext& ctx)
{
	const int bodyHeight = body_height(input.screenHeight);
	const size_t visibleListRows = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t visibleFields = whole_rows_within(bodyHeight, FIELD_HEIGHT);
	const Vector2D mouse = input.mouse;

	// A click in the header is on neither panel.
	if (input.is_pressed(EditorKey::CLICK) && mouse.y >= HEADER_HEIGHT)
	{
		const int pixelsBelowTop = mouse.y - HEADER_HEIGHT;
		if (mouse.x < LIST_WIDTH)
		{
			focus = EditorFocus::LIST;
			const std::optional<size_t> row = listCursor.row_under(
				whole_rows_within(pixelsBelowTop, ITEM_HEIGHT),
				visibleListRows);
			if (row && *row != listCursor.current_row())
			{
				commit_working(ctx);
				listCursor.select(*row);
				load_working(ctx);
			}
		}
		else if (mode == EditorMode::NORMAL)
		{
			focus = EditorFocus::FIELDS;
			const std::optional<size_t> field = fieldCursor.row_under(
				whole_rows_within(pixelsBelowTop, FIELD_HEIGHT),
				visibleFields);
			if (field)
			{
				fieldCursor.select(*field);
			}
		}
	}

	if (input.wheel != 0)
	{
		if (mouse.x < LIST_WIDTH)
		{
			listCursor.scroll_by(-input.wheel, visibleListRows);
		}
		else
		{
			fieldCursor.scroll_by(-input.wheel, visibleFields);
		}
	}

	// Keyboard: list panel
	if (focus == EditorFocus::LIST)
	{
		if (input.is_pressed(EditorKey::UP) && listCursor.can_move_up())
		{
			commit_working(ctx);
			listCursor.move_up();
			load_working(ctx);
		}
		// A screen too short for one row has nowhere to scroll the cursor to.
		else if (
			input.is_pressed(EditorKey::DOWN) && listCursor.can_move_down() &&
			visibleListRows > 0)
		{
			commit_working(ctx);
			listCursor.move_down(visibleListRows);
			load_working(ctx);
		}
		else if (
			input.is_pressed(EditorKey::ENTER) ||
			input.is_pressed(EditorKey::RIGHT))
		{
			focus = EditorFocus::FIELDS;
		}
		else if (input.is_pressed(EditorKey::ADD))
		{
			commit_working(ctx);
			const std::string newKey = model.add_record(ctx);
			keys = model.keys(ctx);
			listCursor.set_row_count(keys.size());
			const auto found = std::ranges::find(keys, newKey);
			assert(
				found != keys.end() &&
				"RecordEditor::handle_normal: the added record is not listed");
			listCursor.select(static_cast<size_t>(found - keys.begin()));
			load_working(ctx);
			focus = EditorFocus::FIELDS;
		}
		else if (input.is_pressed(EditorKey::REMOVE))
		{
			if (!model.is_builtin(ctx, current_key()))
			{
				model.remove_record(ctx, current_key());
				keys = model.keys(ctx);
				listCursor.set_row_count(keys.size());
				load_working(ctx);
			}
		}
		return;
	}

	// Keyboard: field panel
	if (input.is_pressed(EditorKey::UP) && fieldCursor.can_move_up())
	{
		fieldCursor.move_up();
	}
	else if (
		input.is_pressed(EditorKey::DOWN) && fieldCursor.can_move_down() &&
		visibleFields > 0)
	{
		fieldCursor.move_down(visibleFields);
	}

	const FieldId fieldId = current_field();
	const FieldKind kind = descriptor_of(Model::FIELDS, fieldId).kind;

	const bool isTileChosen =
		kind == FieldKind::TILE && input.is_pressed(EditorKey::ENTER);
	if (input.is_pressed(EditorKey::PICKER) || isTileChosen)
	{
		mode = EditorMode::TILE_PICKER;
		return;
	}

	if (input.is_pressed(EditorKey::ENTER))
	{
		if (kind == FieldKind::TEXT)
		{
			editBuffer = model.field_value(fieldId);
			mode = EditorMode::EDIT_STRING;
		}
		else if (kind == FieldKind::TOGGLE)
		{
			model.field_toggle(fieldId);
		}
		return;
	}

	if (kind == FieldKind::ADJUST)
	{
		const int delta = input.is_held(EditorKey::CONTROL) ? 10 : 1;
		if (input.is_pressed(EditorKey::LEFT))
		{
			model.field_adjust(fieldId, -delta);
		}
		else if (input.is_pressed(EditorKey::RIGHT))
		{
			model.field_adjust(fieldId, delta);
		}
	}
}

template <typename Model>
void RecordEditor<Model>::handle_edit_string(const EditorInput& input)
{
	editBuffer += input.typed;

	if (input.is_pressed(EditorKey::BACKSPACE) && !editBuffer.empty())
	{
		editBuffer.pop_back();
	}

	if (input.is_pressed(EditorKey::ENTER))
	{
		model.field_set_string(current_field(), editBuffer);
		mode = EditorMode::NORMAL;
	}

	if (input.is_pressed(EditorKey::ESCAPE))
	{
		mode = EditorMode::NORMAL;
	}
}

template <typename Model>
void RecordEditor<Model>::handle_picker(
	const EditorInput& input,
	const Renderer& renderer)
{
	const int totalSheets = renderer.get_loaded_sheet_count();

	const auto advance_sheet = [&](int dir)
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

	if (input.is_pressed(EditorKey::LEFT))
	{
		advance_sheet(-1);
	}
	if (input.is_pressed(EditorKey::RIGHT))
	{
		advance_sheet(1);
	}
	if (input.is_pressed(EditorKey::ESCAPE) ||
		input.is_pressed(EditorKey::PICKER))
	{
		mode = EditorMode::NORMAL;
		return;
	}

	const int sheetRows =
		renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const Vector2D mouse = input.mouse;
	const bool isMouseInPicker = mouse.x >= LIST_WIDTH;

	if (isMouseInPicker && input.wheel != 0)
	{
		pickerScroll = std::clamp(
			pickerScroll - input.wheel,
			0,
			std::max(0, sheetRows - 1));
	}

	const int gridY = HEADER_HEIGHT + PICKER_SUB_HEADER_HEIGHT;

	if (input.is_pressed(EditorKey::CLICK) && isMouseInPicker)
	{
		const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
		const int col =
			(mouse.x - LIST_WIDTH - PICKER_PAD) / (pickerTileSize + 2);
		const int row = pickerScroll + (mouse.y - gridY) / (pickerTileSize + 2);
		const int sheetColumns =
			renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));

		if (col >= 0 && col < sheetColumns && row >= 0 && row < sheetRows)
		{
			model.set_working_tile(
				TileRef{ static_cast<TileSheet>(pickerSheet), col, row });
			mode = EditorMode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

template <typename Model>
void RecordEditor<Model>::render(
	const EditorInput& input,
	const GameContext& ctx) const
{
	const Renderer& renderer = *ctx.renderer;
	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(renderer);
	render_list(input, ctx);

	if (mode == EditorMode::TILE_PICKER)
	{
		render_picker(input, renderer);
	}
	else
	{
		render_fields(input, renderer);
	}

	render_hint(renderer);
}

template <typename Model>
void RecordEditor<Model>::render_header(const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	const int screenWidth = renderer.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, look.headerBackground);
	renderer.draw_text_color(Vector2D{ 8, 6 }, look.title, look.titleText);
	renderer.draw_text_color(
		Vector2D{ 8, 26 },
		look.headerHint,
		look.headerHintText);
}

template <typename Model>
void RecordEditor<Model>::render_list(
	const EditorInput& input,
	const GameContext& ctx) const
{
	const EditorLook& look = Model::LOOK;
	const Renderer& renderer = *ctx.renderer;
	const int bodyY = HEADER_HEIGHT;
	const int bodyHeight = body_height(renderer.get_screen_height());

	DrawRectangle(0, bodyY, LIST_WIDTH, bodyHeight, look.listBackground);
	DrawLine(
		LIST_WIDTH,
		bodyY,
		LIST_WIDTH,
		bodyY + bodyHeight,
		look.panelDivider);

	const size_t visibleCount = whole_rows_within(bodyHeight, ITEM_HEIGHT);
	const size_t scroll = listCursor.first_visible(visibleCount);
	const Vector2D mouse = input.mouse;

	for (size_t rowIndex = scroll; rowIndex < keys.size(); ++rowIndex)
	{
		const int rowY =
			bodyY + static_cast<int>(rowIndex - scroll) * ITEM_HEIGHT;
		if (rowY + ITEM_HEIGHT > bodyY + bodyHeight)
		{
			break;
		}

		// A line at the panel's top edge would mark nothing.
		if (rowIndex > scroll && has_divider_above(ctx, rowIndex))
		{
			DrawLine(
				LIST_PAD,
				rowY,
				LIST_WIDTH - LIST_PAD,
				rowY,
				CUSTOM_DIVIDER);
		}

		const bool isSelected = (rowIndex == listCursor.current_row());
		const bool hovered = mouse.x >= 0 && mouse.x < LIST_WIDTH &&
			mouse.y >= rowY && mouse.y < rowY + ITEM_HEIGHT;

		Color background{ 0, 0, 0, 0 };
		if (isSelected && focus == EditorFocus::LIST)
		{
			background = look.listRowFocused;
		}
		else if (isSelected)
		{
			background = look.listRowSelected;
		}
		else if (hovered)
		{
			background = look.listRowHovered;
		}

		if (background.a > 0)
		{
			DrawRectangle(0, rowY, LIST_WIDTH, ITEM_HEIGHT, background);
		}

		const std::string& key = keys[rowIndex];
		const TileRef tile =
			isSelected ? model.working_tile() : model.row_tile(ctx, key);
		renderer.draw_tile_screen_sized(
			Vector2D{ LIST_PAD, rowY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 },
			tile,
			LIST_TILE_SIZE);

		std::string label{};
		if (isSelected)
		{
			label = model.working_row_label(key);
		}
		else
		{
			label = model.row_label(ctx, key);
		}
		if (!model.is_builtin(ctx, key))
		{
			label += " *";
		}

		renderer.draw_text_color(
			Vector2D{
				LIST_PAD + LIST_TILE_SIZE + 4,
				rowY + (ITEM_HEIGHT - 16) / 2,
			},
			label,
			isSelected ? look.listTextSelected : look.listTextPlain);
	}
}

template <typename Model>
void RecordEditor<Model>::render_fields(
	const EditorInput& input,
	const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	const int screenWidth = renderer.get_screen_width();
	const int panelX = LIST_WIDTH;
	const int panelY = HEADER_HEIGHT;
	const int fieldsWidth = screenWidth - LIST_WIDTH;
	const int fieldsHeight = body_height(renderer.get_screen_height());

	DrawRectangle(
		panelX,
		panelY,
		fieldsWidth,
		fieldsHeight,
		look.fieldsBackground);

	const size_t visible = whole_rows_within(fieldsHeight, FIELD_HEIGHT);
	const size_t scroll = fieldCursor.first_visible(visible);
	const Vector2D mouse = input.mouse;

	for (
		size_t fieldIndex = scroll; fieldIndex < Model::FIELD_COUNT;
		++fieldIndex)
	{
		const int rowY =
			panelY + static_cast<int>(fieldIndex - scroll) * FIELD_HEIGHT;
		if (rowY + FIELD_HEIGHT > panelY + fieldsHeight)
		{
			break;
		}

		const FieldId fieldId = Model::FIELDS.at(fieldIndex).field;
		const bool isSelected = (fieldIndex == fieldCursor.current_row());
		const bool hovered = mouse.x >= panelX && mouse.x < screenWidth &&
			mouse.y >= rowY && mouse.y < rowY + FIELD_HEIGHT;

		Color background{ 0, 0, 0, 0 };
		if (isSelected && focus == EditorFocus::FIELDS)
		{
			background = look.fieldRowFocused;
		}
		else if (isSelected)
		{
			background = look.fieldRowSelected;
		}
		else if (hovered)
		{
			background = look.fieldRowHovered;
		}

		if (background.a > 0)
		{
			DrawRectangle(panelX, rowY, fieldsWidth, FIELD_HEIGHT, background);
		}

		renderer.draw_text_color(
			Vector2D{ panelX + 12, rowY + (FIELD_HEIGHT - 16) / 2 },
			model.field_label(fieldId),
			isSelected ? look.labelSelected : look.labelPlain);

		if (descriptor_of(Model::FIELDS, fieldId).kind == FieldKind::TILE)
		{
			const Vector2D tileAt{
				screenWidth - LIST_TILE_SIZE - 12,
				rowY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2,
			};
			renderer.draw_tile_screen_sized(
				tileAt,
				model.working_tile(),
				LIST_TILE_SIZE);
			if (isSelected)
			{
				DrawRectangleLines(
					tileAt.x,
					tileAt.y,
					LIST_TILE_SIZE,
					LIST_TILE_SIZE,
					look.tileOutline);
			}
		}
		else
		{
			const std::string valueText =
				(isSelected && mode == EditorMode::EDIT_STRING) ?
				editBuffer + "_" :
				model.field_value(fieldId);
			const int valuePixelX =
				screenWidth - renderer.measure_text(valueText) - 16;
			renderer.draw_text_color(
				Vector2D{ valuePixelX, rowY + (FIELD_HEIGHT - 16) / 2 },
				valueText,
				isSelected ? look.valueSelected : look.valuePlain);
		}
	}
}

template <typename Model>
void RecordEditor<Model>::render_picker(
	const EditorInput& input,
	const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	const int screenWidth = renderer.get_screen_width();
	const int panelX = LIST_WIDTH;
	const int panelWidth = screenWidth - LIST_WIDTH;
	const int bodyY = HEADER_HEIGHT;
	const int bodyHeight = body_height(renderer.get_screen_height());

	DrawRectangle(panelX, bodyY, panelWidth, bodyHeight, look.pickerBackground);

	DrawRectangle(
		panelX,
		bodyY,
		panelWidth,
		PICKER_SUB_HEADER_HEIGHT,
		look.pickerSubHeader);
	const int totalSheets = renderer.get_loaded_sheet_count();
	const std::string header = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		renderer.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		totalSheets);
	renderer.draw_text_color(
		Vector2D{ panelX + PICKER_PAD, bodyY + 6 },
		header,
		look.pickerHeaderText);

	const int gridY = bodyY + PICKER_SUB_HEADER_HEIGHT;
	const int gridHeight = bodyHeight - PICKER_SUB_HEADER_HEIGHT;
	const int sheetColumns =
		renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));
	const int sheetRows =
		renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
	const Vector2D mouse = input.mouse;
	const TileRef currentTile = model.working_tile();

	BeginScissorMode(panelX, gridY, panelWidth, gridHeight);

	for (int row = pickerScroll; row < sheetRows; ++row)
	{
		const int cellPixelY =
			gridY + (row - pickerScroll) * (pickerTileSize + 2);
		if (cellPixelY >= gridY + gridHeight)
		{
			break;
		}

		for (int col = 0; col < sheetColumns; ++col)
		{
			const int cellPixelX =
				panelX + PICKER_PAD + col * (pickerTileSize + 2);
			const TileRef tile{ static_cast<TileSheet>(pickerSheet), col, row };

			const bool isCurrentTile = (tile == currentTile);
			const bool hovered = mouse.x >= cellPixelX &&
				mouse.x < cellPixelX + pickerTileSize &&
				mouse.y >= cellPixelY && mouse.y < cellPixelY + pickerTileSize;

			if (isCurrentTile)
			{
				DrawRectangle(
					cellPixelX,
					cellPixelY,
					pickerTileSize,
					pickerTileSize,
					look.pickerCurrent);
			}
			else if (hovered)
			{
				DrawRectangle(
					cellPixelX,
					cellPixelY,
					pickerTileSize,
					pickerTileSize,
					look.pickerHovered);
			}

			renderer.draw_tile_screen_sized(
				Vector2D{ cellPixelX, cellPixelY },
				tile,
				pickerTileSize);

			if (isCurrentTile)
			{
				DrawRectangleLines(
					cellPixelX,
					cellPixelY,
					pickerTileSize,
					pickerTileSize,
					look.tileOutline);
			}
		}
	}

	EndScissorMode();
}

template <typename Model>
void RecordEditor<Model>::render_hint(const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	const int screenWidth = renderer.get_screen_width();
	const int hintY = renderer.get_screen_height() - HINT_HEIGHT;

	DrawRectangle(0, hintY, screenWidth, HINT_HEIGHT, look.hintBackground);

	const bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;
	std::string_view hintText{};
	if (isSaveRecent)
	{
		hintText = "Saved!";
	}
	else if (mode == EditorMode::EDIT_STRING)
	{
		hintText = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (mode == EditorMode::TILE_PICKER)
	{
		hintText =
			"Tile Picker  --  Left/Right:sheet  Click:assign  Esc/F2:back";
	}
	else if (focus == EditorFocus::LIST)
	{
		hintText = look.listHint;
	}
	else
	{
		hintText = look.fieldsHint;
	}

	renderer.draw_text_color(
		Vector2D{ 8, hintY + 6 },
		hintText,
		isSaveRecent ? look.savedHint : look.plainHint);
}

// ---------------------------------------------------------------------------
// Cursor
// ---------------------------------------------------------------------------

template <typename Model>
const std::string& RecordEditor<Model>::current_key() const
{
	if (keys.empty())
	{
		throw std::out_of_range(
			"RecordEditor::current_key -- key list is empty");
	}
	return keys[listCursor.current_row()];
}

template <typename Model>
bool RecordEditor<Model>::has_divider_above(
	const GameContext& ctx,
	size_t row) const
{
	return row > 0 && row < keys.size() &&
		model.is_builtin(ctx, keys[row - 1]) &&
		!model.is_builtin(ctx, keys[row]);
}

template <typename Model>
typename RecordEditor<Model>::FieldId RecordEditor<Model>::current_field() const
{
	return Model::FIELDS.at(fieldCursor.current_row()).field;
}

template class RecordEditor<ItemEditorModel>;
