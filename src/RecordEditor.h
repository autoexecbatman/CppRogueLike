#pragma once

// file: RecordEditor.h
//
// The frame every data editor shares: a list of records on the left, the
// selected record's fields on the right, a tile picker, a header and a hint
// bar, driven by the keys and mouse in one EditorInput per frame. Each editor
// is RecordEditor over a Model, which supplies the record type, its field
// table, its colours and how records are read, written, added and removed.
// The frame knows no record's fields.
//
// Usage:
//
//   ItemEditor items{};    // RecordEditor<ItemEditorModel>
//   items.enter(ctx);      // lists every item, the first selected
//   items.tick(ctx);       // one frame: real input, then drawing
//
//   // a test drives the input alone; no window is needed for it
//   EditorInput input{};
//   input.pressed = { EditorKey::TAB };   // focus moves to the fields
//   input.screenHeight = 900;
//   items.handle_input(input, ctx);
//   items.editor_focus();                 // -> EditorFocus::FIELDS
//   items.exit(ctx);    // commits the record, opens the main menu
//
// Controls:
//   Tab              switch focus between the list and the fields
//   Up / Down        move through the list or the fields
//   Left / Right     step a number or a cycled choice; Ctrl steps by ten
//   Enter            type into a text field, step a choice, or open the picker
//   F2               open the tile picker, where records have tiles
//   A / Delete       add a record, remove a custom one (list focused)
//   Ctrl+S           save
//   Esc              commit the record and leave
//
// While a field is being typed into or the picker is open, Esc only cancels
// it, and Tab and Ctrl+S do nothing. A field the Model reports read-only for
// the record, such as a builtin spell's Effect, ignores Enter and the arrows.
//
// The member definitions are in RecordEditor.cpp, which instantiates the frame
// once per Model; a new editor adds its line there.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <raylib.h>

#include "EditorField.h"
#include "EditorInput.h"
#include "ListCursor.h"
#include "TileRef.h"

struct GameContext;
class Renderer;

// What the editor's keys act on.
enum class EditorMode
{
	// Keys move the cursors and edit fields.
	NORMAL,
	// Keys type into the selected text field.
	EDIT_STRING,
	// The field panel shows a tile sheet to pick from.
	TILE_PICKER
};

// Which panel the arrows move in.
enum class EditorFocus
{
	LIST,
	FIELDS
};

// Where the frame draws, in pixels.
namespace EditorLayout
{
	inline constexpr int LIST_TILE_SIZE = 22;
	// The width a list row's tile takes, with the gap before its label.
	inline constexpr int LIST_TILE_COLUMN = LIST_TILE_SIZE + 4;
	inline constexpr int LIST_PAD = 6;
	inline constexpr int PICKER_PAD = 10;
	// The drawn cell size the picker's grid aims for.
	inline constexpr int PICKER_TILE_TARGET = 64;
	// Pixels between a line's box and the edge of the band holding it.
	inline constexpr int TEXT_PAD = 2;

	// The height of a band holding lines of text, each lineHeight tall, with
	// TEXT_PAD above and below.
	//
	// Example:
	//   text_band_height(30, 1);   // -> 34
	//   text_band_height(30, 2);   // -> 64
	[[nodiscard]] constexpr int text_band_height(int lineHeight, int lines)
	{
		return lines * lineHeight + 2 * TEXT_PAD;
	}

	// The header's height: its title line and its key line.
	//
	// Example:
	//   header_height(30);   // -> 64
	[[nodiscard]] constexpr int header_height(int lineHeight)
	{
		return text_band_height(lineHeight, 2);
	}

	// The height of a one-line bar: the hint bar and the picker's sub-header.
	//
	// Example:
	//   hint_height(30);   // -> 34
	[[nodiscard]] constexpr int hint_height(int lineHeight)
	{
		return text_band_height(lineHeight, 1);
	}

	// A list or field row's height: one line, or a list tile when the tile is
	// the taller.
	//
	// Example:
	//   row_height(30);   // -> 34
	//   row_height(16);   // -> 26, the tile's 22 and its pads
	[[nodiscard]] constexpr int row_height(int lineHeight)
	{
		// The tile floor would hide a missing line height, so it is refused
		// here.
		assert(lineHeight > 0 && "EditorLayout::row_height: no line height");
		return std::max(
			text_band_height(lineHeight, 1),
			LIST_TILE_SIZE + 2 * TEXT_PAD);
	}

	// The height of the panels between the header and the hint bar. Input and
	// drawing both read it, so a click and the row drawn under it come from one
	// row count.
	//
	// Example:
	//   body_height(1061, 30);   // -> 963, after a 64 header and a 34 hint bar
	[[nodiscard]] constexpr int body_height(int screenHeight, int lineHeight)
	{
		assert(lineHeight > 0 && "EditorLayout::body_height: no line height");
		return screenHeight - header_height(lineHeight) -
			hint_height(lineHeight);
	}

	// The top of the header's line, counted from 0.
	//
	// Example:
	//   header_line_top(1, 30);   // -> 32
	[[nodiscard]] constexpr int header_line_top(int line, int lineHeight)
	{
		return TEXT_PAD + line * lineHeight;
	}

	// The top of a panel's row, counted from 0 at the panel's top.
	//
	// Example:
	//   row_top(64, 3, 34);   // -> 166
	[[nodiscard]] constexpr int row_top(int bodyTop, int row, int rowHeight)
	{
		return bodyTop + row * rowHeight;
	}

	// The list panel's width: a pad, the tile column, the widest label, the tag
	// column and a pad. It is at most half the screen, because the field
	// panel's right-aligned values need the other half.
	//
	// Example:
	//   list_panel_width(200, 26, 30, 1920);   // -> 268
	//   list_panel_width(2000, 0, 0, 1920);    // -> 960, half the screen
	[[nodiscard]] constexpr int list_panel_width(
		int widestLabel,
		int tileColumn,
		int tagColumn,
		int screenWidth)
	{
		return std::min(
			LIST_PAD + tileColumn + widestLabel + tagColumn + LIST_PAD,
			screenWidth / 2);
	}

	// The width the list keeps for row tags: the widest tag and a pad, or none
	// when no row has a tag.
	//
	// Example:
	//   tag_column(10);   // -> 16
	//   tag_column(0);    // -> 0
	[[nodiscard]] constexpr int tag_column(int widestTag)
	{
		return widestTag > 0 ? widestTag + LIST_PAD : 0;
	}

	// Where a row's tag starts: the tag column's left edge, inside the list.
	//
	// Example:
	//   tag_left(218, 14);   // -> 204
	[[nodiscard]] constexpr int tag_left(int listWidth, int tagColumn)
	{
		return listWidth - tagColumn;
	}

	// Where a line of text starts so that it sits centred in a band. A line
	// taller than the band overhangs it equally above and below.
	//
	// Example:
	//   centred_text_top(1033, 28, 30);   // -> 1032, a 30-pixel line in 28
	//   centred_text_top(0, 30, 16);      // -> 7
	[[nodiscard]] constexpr int centred_text_top(
		int bandTop,
		int bandHeight,
		int lineHeight)
	{
		return bandTop + (bandHeight - lineHeight) / 2;
	}

	// The label if it fits maxWidth, otherwise its longest prefix followed by
	// "..." that fits, and "" when not even "..." fits.
	//
	// Example, measuring 8 pixels a character:
	//   fitted_label("Bless", 80, measure);                  // -> "Bless"
	//   fitted_label("Protection From Evil", 80, measure);   // -> "Protect..."
	//   fitted_label("Bless", 20, measure);                  // -> ""
	[[nodiscard]] std::string fitted_label(
		std::string_view label,
		int maxWidth,
		const std::function<int(std::string_view)>& measure);
} // namespace EditorLayout

// An editor's colours and the words on its header and hint bar.
struct EditorLook
{
	std::string_view title{};
	std::string_view headerHint{};
	std::string_view listHint{};
	std::string_view fieldsHint{};

	Color headerBackground{};
	Color titleText{};
	Color headerHintText{};

	Color listBackground{};
	Color panelDivider{};
	Color listRowFocused{};
	Color listRowSelected{};
	Color listRowHovered{};
	Color listTextSelected{};
	Color listTextPlain{};

	Color fieldsBackground{};
	Color fieldRowFocused{};
	Color fieldRowSelected{};
	Color fieldRowHovered{};
	Color labelSelected{};
	Color labelPlain{};
	Color valueSelected{};
	Color valuePlain{};
	Color tileOutline{};

	Color pickerBackground{};
	Color pickerSubHeader{};
	Color pickerHeaderText{};
	Color pickerCurrent{};
	Color pickerHovered{};

	Color hintBackground{};
	Color savedHint{};
	Color plainHint{};
};

// A short mark a list row carries at its right edge, such as a spell's class.
struct RowTag
{
	std::string_view text{};
	Color colour{};
};

// The shared editor frame over one Model; see the file comment for use.
template <typename Model>
class RecordEditor
{
private:
	using FieldId = typename Model::FieldId;

	// A Model with a TILE field has a tile picker and a tile column in its
	// list; one without has neither, and supplies no tile functions.
	static constexpr bool HAS_TILES = has_tile_field(Model::FIELDS);

	// The record type's own knowledge: its fields, colours and registry.
	Model model{};
	bool active{ false };
	EditorMode mode{ EditorMode::NORMAL };
	EditorFocus focus{ EditorFocus::LIST };

	// Every record's key, in the list's order.
	std::vector<std::string> keys{};
	ListCursor listCursor{};
	ListCursor fieldCursor{};
	// The text being typed into a field.
	std::string editBuffer{};

	// The tile sheet the picker shows, and its first visible row.
	int pickerSheet{ 0 };
	int pickerScroll{ 0 };

	// When the last save happened, for the hint bar's "Saved!".
	double lastSaveTime{ -100.0 };

	void load_working(const GameContext& ctx);
	void commit_working(GameContext& ctx);

	void handle_normal(const EditorInput& input, GameContext& ctx);
	void handle_edit_string(const EditorInput& input);
	void handle_picker(
		const EditorInput& input,
		const Renderer& renderer,
		int listWidth);

	void render(const EditorInput& input, const GameContext& ctx) const;
	void render_header(
		const EditorInput& input,
		const Renderer& renderer) const;
	void render_list(
		const EditorInput& input,
		const GameContext& ctx,
		int listWidth) const;
	void render_fields(
		const EditorInput& input,
		const Renderer& renderer,
		int listWidth) const;
	void render_picker(
		const EditorInput& input,
		const Renderer& renderer,
		int listWidth) const;
	void render_hint(const EditorInput& input, const Renderer& renderer) const;

	// Row's label as the list draws it: the working record's for the
	// selected row, with " *" after a custom record.
	[[nodiscard]] std::string row_label_as_drawn(
		const GameContext& ctx,
		size_t row) const;

	// The widest row tag as the list draws it, in pixels; 0 when no row
	// has one. Requires input.measure.
	[[nodiscard]] int widest_tag(
		const EditorInput& input,
		const GameContext& ctx) const;

	// The selected record's tile, or no tile for a Model without tiles.
	[[nodiscard]] TileRef working_tile() const;

public:
	// Lists every record and selects the first. Requires the Model's registry
	// in ctx.
	void enter(GameContext& ctx);
	// Commits the selected record, deactivates and opens the main menu.
	void exit(GameContext& ctx);
	// One frame: reads the real input, acts on it, then draws.
	void tick(GameContext& ctx);
	// Acts on one frame's input. Saving writes the Model's files on disk.
	void handle_input(const EditorInput& input, GameContext& ctx);

	[[nodiscard]] bool is_active() const { return active; }

	[[nodiscard]] EditorMode editor_mode() const { return mode; }

	[[nodiscard]] EditorFocus editor_focus() const { return focus; }

	// The selected record's key. Throws std::out_of_range when there is none.
	[[nodiscard]] const std::string& current_key() const;
	// The field the cursor is on, as the Model's table names that row.
	[[nodiscard]] FieldId current_field() const;

	// The list panel's width this frame, from the widest label and tag as
	// the list draws them. Requires input.measure.
	//
	// Example, measuring 8 pixels a character:
	//   spells.list_width(input, ctx);   // -> 218
	//   // the widest label, "[1] Protection From Evil", is 192 of it
	[[nodiscard]] int list_width(
		const EditorInput& input,
		const GameContext& ctx) const;

	// The Model, for reading the working record's fields.
	[[nodiscard]] const Model& record_model() const { return model; }

	// Whether the list draws its line above row: the first custom record after
	// a shipped one. A Model lists shipped records first, so there is one.
	//
	// Example:
	//   has_divider_above(ctx, 0);             // -> false, nothing above it
	//   has_divider_above(ctx, firstCustom);   // -> true
	[[nodiscard]] bool has_divider_above(
		const GameContext& ctx,
		size_t row) const;
};
