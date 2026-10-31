// file: RecordEditor.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <raylib.h>

#include "EditorField.h"
#include "EditorInput.h"
#include "GameContext.h"
#include "ItemEditor.h"
#include "ListCursor.h"
#include "Menu.h"
#include "MonsterEditor.h"
#include "RecordEditor.h"
#include "Renderer.h"
#include "SpellEditor.h"
#include "TileRef.h"
#include "Vector2D.h"

namespace CppRL
{

using namespace EditorLayout;

std::string EditorLayout::fitted_label(
	std::string_view label,
	int maxWidth,
	const std::function<int(std::string_view)>& measure)
{
	if (measure(label) <= maxWidth)
	{
		return std::string{ label };
	}
	// Longest prefix first, so the first that fits is the one kept.
	for (size_t length = label.size(); length > 0; --length)
	{
		std::string cut = std::string{ label.substr(0, length - 1) } + "...";
		if (measure(cut) <= maxWidth)
		{
			return cut;
		}
	}
	return "";
}

namespace
{
	// The line drawn where the shipped records end and the custom ones begin.
	constexpr Color CUSTOM_DIVIDER{ 70, 70, 70, 200 };

	// Largest integer multiple of the sheet's own cell size that fits the
	// target, so a 16-pixel tile draws at 4x and a 64-pixel tile at 1x - never
	// blurred.
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
	// Typing and the picker own every key, Esc included, so Esc cancels them.
	if (mode == EditorMode::EDIT_STRING)
	{
		handle_edit_string(input);
		return;
	}

	if (mode == EditorMode::TILE_PICKER)
	{
		assert(
			HAS_TILES &&
			"RecordEditor::handle_input: a picker opened for records without tiles");
		assert(
			ctx.renderer &&
			"RecordEditor::handle_input: the tile picker needs a renderer");
		handle_picker(input, *ctx.renderer, list_width(input, ctx));
		return;
	}

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

	handle_normal(input, ctx);
}

template <typename Model>
void RecordEditor<Model>::handle_normal(
	const EditorInput& input,
	GameContext& ctx)
{
	const int bodyTop = header_height(input.lineHeight);
	const int rowHeight = row_height(input.lineHeight);
	const size_t visibleRows = whole_rows_within(
		body_height(input.screenHeight, input.lineHeight),
		rowHeight);
	const Vector2D mouse = input.mouse;
	const int listWidth = list_width(input, ctx);

	// A click in the header is on neither panel.
	if (input.is_pressed(EditorKey::CLICK) && mouse.y >= bodyTop)
	{
		const int pixelsBelowTop = mouse.y - bodyTop;
		if (mouse.x < listWidth)
		{
			focus = EditorFocus::LIST;
			const std::optional<size_t> row = listCursor.row_under(
				whole_rows_within(pixelsBelowTop, rowHeight),
				visibleRows);
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
				whole_rows_within(pixelsBelowTop, rowHeight),
				visibleRows);
			if (field)
			{
				fieldCursor.select(*field);
			}
		}
	}

	if (input.wheel != 0)
	{
		if (mouse.x < listWidth)
		{
			listCursor.scroll_by(-input.wheel, visibleRows);
		}
		else
		{
			fieldCursor.scroll_by(-input.wheel, visibleRows);
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
			visibleRows > 0)
		{
			commit_working(ctx);
			listCursor.move_down(visibleRows);
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
		visibleRows > 0)
	{
		fieldCursor.move_down(visibleRows);
	}

	const FieldId fieldId = current_field();
	const FieldKind kind = descriptor_of(Model::FIELDS, fieldId).kind;

	const bool isTileChosen =
		kind == FieldKind::TILE && input.is_pressed(EditorKey::ENTER);
	const bool isPickerAsked =
		input.is_pressed(EditorKey::PICKER) || isTileChosen;
	if (HAS_TILES && isPickerAsked)
	{
		mode = EditorMode::TILE_PICKER;
		return;
	}

	// A field the record holds fixed takes no edit key.
	if (model.is_read_only(fieldId))
	{
		return;
	}

	if (input.is_pressed(EditorKey::ENTER))
	{
		if (kind == FieldKind::TEXT)
		{
			editBuffer = model.field_value(fieldId);
			mode = EditorMode::EDIT_STRING;
		}
		else if (kind == FieldKind::TOGGLE || kind == FieldKind::CYCLE)
		{
			model.field_toggle(fieldId);
		}
		return;
	}

	if (kind == FieldKind::ADJUST || kind == FieldKind::CYCLE)
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
	const Renderer& renderer,
	int listWidth)
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
	const bool isMouseInPicker = mouse.x >= listWidth;

	if (isMouseInPicker && input.wheel != 0)
	{
		pickerScroll = std::clamp(
			pickerScroll - input.wheel,
			0,
			std::max(0, sheetRows - 1));
	}

	const int gridY =
		header_height(input.lineHeight) + hint_height(input.lineHeight);

	if (input.is_pressed(EditorKey::CLICK) && isMouseInPicker)
	{
		const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
		const int col =
			(mouse.x - listWidth - PICKER_PAD) / (pickerTileSize + 2);
		const int row = pickerScroll + (mouse.y - gridY) / (pickerTileSize + 2);
		const int sheetColumns =
			renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));

		if (col >= 0 && col < sheetColumns && row >= 0 && row < sheetRows)
		{
			if constexpr (HAS_TILES)
			{
				model.set_working_tile(
					TileRef{ static_cast<TileSheet>(pickerSheet), col, row });
			}
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

	// Every size comes from the input snapshot that handle_input read.
	DrawRectangle(
		0,
		0,
		input.screenWidth,
		input.screenHeight,
		Color{ 0, 0, 0, 255 });

	// One width serves the list and the panel beside it.
	const int listWidth = list_width(input, ctx);
	render_header(input, renderer);
	render_list(input, ctx, listWidth);

	if (mode == EditorMode::TILE_PICKER)
	{
		render_picker(input, renderer, listWidth);
	}
	else
	{
		render_fields(input, renderer, listWidth);
	}

	render_hint(input, renderer);
}

template <typename Model>
void RecordEditor<Model>::render_header(
	const EditorInput& input,
	const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	DrawRectangle(
		0,
		0,
		input.screenWidth,
		header_height(input.lineHeight),
		look.headerBackground);
	renderer.draw_text_color(
		Vector2D{ 8, header_line_top(0, input.lineHeight) },
		look.title,
		look.titleText);
	renderer.draw_text_color(
		Vector2D{ 8, header_line_top(1, input.lineHeight) },
		look.headerHint,
		look.headerHintText);
}

template <typename Model>
void RecordEditor<Model>::render_list(
	const EditorInput& input,
	const GameContext& ctx,
	int listWidth) const
{
	const EditorLook& look = Model::LOOK;
	const Renderer& renderer = *ctx.renderer;
	const int bodyY = header_height(input.lineHeight);
	const int bodyHeight = body_height(input.screenHeight, input.lineHeight);
	const int rowHeight = row_height(input.lineHeight);

	DrawRectangle(0, bodyY, listWidth, bodyHeight, look.listBackground);
	DrawLine(
		listWidth,
		bodyY,
		listWidth,
		bodyY + bodyHeight,
		look.panelDivider);

	const size_t visibleCount = whole_rows_within(bodyHeight, rowHeight);
	const int tagColumn = tag_column(widest_tag(input, ctx));
	const size_t scroll = listCursor.first_visible(visibleCount);
	const Vector2D mouse = input.mouse;

	for (size_t rowIndex = scroll; rowIndex < keys.size(); ++rowIndex)
	{
		const int rowY =
			row_top(bodyY, static_cast<int>(rowIndex - scroll), rowHeight);
		if (rowY + rowHeight > bodyY + bodyHeight)
		{
			break;
		}

		// A line at the panel's top edge would mark nothing.
		if (rowIndex > scroll && has_divider_above(ctx, rowIndex))
		{
			DrawLine(
				LIST_PAD,
				rowY,
				listWidth - LIST_PAD,
				rowY,
				CUSTOM_DIVIDER);
		}

		const bool isSelected = (rowIndex == listCursor.current_row());
		const bool hovered = mouse.x >= 0 && mouse.x < listWidth &&
			mouse.y >= rowY && mouse.y < rowY + rowHeight;

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
			DrawRectangle(0, rowY, listWidth, rowHeight, background);
		}

		const std::string& key = keys[rowIndex];
		int labelX = LIST_PAD;
		if constexpr (HAS_TILES)
		{
			const TileRef tile =
				isSelected ? model.working_tile() : model.row_tile(ctx, key);
			renderer.draw_tile_screen_sized(
				Vector2D{
					LIST_PAD,
					centred_text_top(rowY, rowHeight, LIST_TILE_SIZE),
				},
				tile,
				LIST_TILE_SIZE);
			labelX = LIST_PAD + LIST_TILE_COLUMN;
		}

		// Only a label past the cap is cut; it stops short of the tag.
		const int textY = centred_text_top(rowY, rowHeight, input.lineHeight);
		const std::string label = fitted_label(
			row_label_as_drawn(ctx, rowIndex),
			listWidth - labelX - tagColumn - LIST_PAD,
			input.measure);
		renderer.draw_text_color(
			Vector2D{ labelX, textY },
			label,
			isSelected ? look.listTextSelected : look.listTextPlain);

		const std::optional<RowTag> tag =
			isSelected ? model.working_row_tag() : model.row_tag(ctx, key);
		if (tag)
		{
			renderer.draw_text_color(
				Vector2D{
					tag_left(listWidth, tagColumn),
					textY,
				},
				tag->text,
				tag->colour);
		}
	}
}

template <typename Model>
void RecordEditor<Model>::render_fields(
	const EditorInput& input,
	const Renderer& renderer,
	int listWidth) const
{
	const EditorLook& look = Model::LOOK;
	const int screenWidth = input.screenWidth;
	const int panelX = listWidth;
	const int panelY = header_height(input.lineHeight);
	const int fieldsWidth = screenWidth - listWidth;
	const int fieldsHeight = body_height(input.screenHeight, input.lineHeight);
	const int rowHeight = row_height(input.lineHeight);

	DrawRectangle(
		panelX,
		panelY,
		fieldsWidth,
		fieldsHeight,
		look.fieldsBackground);

	const size_t visible = whole_rows_within(fieldsHeight, rowHeight);
	const size_t scroll = fieldCursor.first_visible(visible);
	const Vector2D mouse = input.mouse;

	for (
		size_t fieldIndex = scroll; fieldIndex < Model::FIELD_COUNT;
		++fieldIndex)
	{
		const int rowY =
			row_top(panelY, static_cast<int>(fieldIndex - scroll), rowHeight);
		if (rowY + rowHeight > panelY + fieldsHeight)
		{
			break;
		}

		const FieldId fieldId = Model::FIELDS.at(fieldIndex).field;
		const bool isSelected = (fieldIndex == fieldCursor.current_row());
		const bool hovered = mouse.x >= panelX && mouse.x < screenWidth &&
			mouse.y >= rowY && mouse.y < rowY + rowHeight;

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
			DrawRectangle(panelX, rowY, fieldsWidth, rowHeight, background);
		}

		const int textY = centred_text_top(rowY, rowHeight, input.lineHeight);
		renderer.draw_text_color(
			Vector2D{ panelX + 12, textY },
			model.field_label(fieldId),
			isSelected ? look.labelSelected : look.labelPlain);

		if (descriptor_of(Model::FIELDS, fieldId).kind == FieldKind::TILE)
		{
			const Vector2D tileAt{
				screenWidth - LIST_TILE_SIZE - 12,
				centred_text_top(rowY, rowHeight, LIST_TILE_SIZE),
			};
			renderer.draw_tile_screen_sized(
				tileAt,
				working_tile(),
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
			// A fixed value is drawn in the label colour, so it reads as fixed.
			const Color valueColor = model.is_read_only(fieldId) ?
				look.labelPlain :
				(isSelected ? look.valueSelected : look.valuePlain);
			renderer.draw_text_color(
				Vector2D{ valuePixelX, textY },
				valueText,
				valueColor);
		}
	}
}

template <typename Model>
void RecordEditor<Model>::render_picker(
	const EditorInput& input,
	const Renderer& renderer,
	int listWidth) const
{
	const EditorLook& look = Model::LOOK;
	const int panelX = listWidth;
	const int panelWidth = input.screenWidth - listWidth;
	const int bodyY = header_height(input.lineHeight);
	const int bodyHeight = body_height(input.screenHeight, input.lineHeight);
	const int subHeaderHeight = hint_height(input.lineHeight);

	DrawRectangle(panelX, bodyY, panelWidth, bodyHeight, look.pickerBackground);

	DrawRectangle(
		panelX,
		bodyY,
		panelWidth,
		subHeaderHeight,
		look.pickerSubHeader);
	const int totalSheets = renderer.get_loaded_sheet_count();
	const std::string header = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		renderer.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		totalSheets);
	renderer.draw_text_color(
		Vector2D{
			panelX + PICKER_PAD,
			centred_text_top(bodyY, subHeaderHeight, input.lineHeight),
		},
		header,
		look.pickerHeaderText);

	const int gridY = bodyY + subHeaderHeight;
	const int gridHeight = bodyHeight - subHeaderHeight;
	const int sheetColumns =
		renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));
	const int sheetRows =
		renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
	const Vector2D mouse = input.mouse;
	const TileRef currentTile = working_tile();

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
void RecordEditor<Model>::render_hint(
	const EditorInput& input,
	const Renderer& renderer) const
{
	const EditorLook& look = Model::LOOK;
	const int hintHeight = hint_height(input.lineHeight);
	const int hintY = input.screenHeight - hintHeight;

	DrawRectangle(0, hintY, input.screenWidth, hintHeight, look.hintBackground);

	const bool isSaveRecent = (GetTime() - lastSaveTime) < 2.0;
	std::string fixedFieldHint{};
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
	else if (model.is_read_only(current_field()))
	{
		fixedFieldHint = std::format(
			"[FIELDS] {} is fixed for this record  Up/Down:navigate  Tab:switch  Ctrl+S:save  Esc:exit",
			model.field_label(current_field()));
		hintText = fixedFieldHint;
	}
	else
	{
		hintText = look.fieldsHint;
	}

	renderer.draw_text_color(
		Vector2D{
			8,
			centred_text_top(hintY, hintHeight, input.lineHeight),
		},
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
TileRef RecordEditor<Model>::working_tile() const
{
	if constexpr (HAS_TILES)
	{
		return model.working_tile();
	}
	else
	{
		return TileRef{};
	}
}

template <typename Model>
typename RecordEditor<Model>::FieldId RecordEditor<Model>::current_field() const
{
	return Model::FIELDS.at(fieldCursor.current_row()).field;
}

template <typename Model>
std::string RecordEditor<Model>::row_label_as_drawn(
	const GameContext& ctx,
	size_t row) const
{
	const std::string& key = keys.at(row);
	std::string label = row == listCursor.current_row() ?
		model.working_row_label(key) :
		model.row_label(ctx, key);
	if (!model.is_builtin(ctx, key))
	{
		label += " *";
	}
	return label;
}

template <typename Model>
int RecordEditor<Model>::widest_tag(
	const EditorInput& input,
	const GameContext& ctx) const
{
	assert(
		input.measure &&
		"RecordEditor::widest_tag: an EditorInput without a measure");
	int widest = 0;
	for (size_t row = 0; row < keys.size(); ++row)
	{
		// The selected row shows the working record, as render_list draws it.
		const std::optional<RowTag> tag = row == listCursor.current_row() ?
			model.working_row_tag() :
			model.row_tag(ctx, keys[row]);
		if (tag)
		{
			widest = std::max(widest, input.measure(tag->text));
		}
	}
	return widest;
}

template <typename Model>
int RecordEditor<Model>::list_width(
	const EditorInput& input,
	const GameContext& ctx) const
{
	assert(
		input.measure &&
		"RecordEditor::list_width: an EditorInput without a measure");
	int widestLabel = 0;
	for (size_t row = 0; row < keys.size(); ++row)
	{
		widestLabel =
			std::max(widestLabel, input.measure(row_label_as_drawn(ctx, row)));
	}
	const int tileColumn = HAS_TILES ? LIST_TILE_COLUMN : 0;
	return list_panel_width(
		widestLabel,
		tileColumn,
		tag_column(widest_tag(input, ctx)),
		input.screenWidth);
}

template class RecordEditor<ItemEditorModel>;
template class RecordEditor<MonsterEditorModel>;
template class RecordEditor<SpellEditorModel>;

} // namespace CppRL
