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
// The member definitions are in RecordEditor.cpp, which instantiates the frame
// once per Model; a new editor adds its line there.

#include <cstddef>
#include <optional>
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
inline constexpr int LIST_WIDTH = 220;
inline constexpr int HEADER_HEIGHT = 48;
inline constexpr int HINT_HEIGHT = 28;
inline constexpr int FIELD_HEIGHT = 26;
inline constexpr int ITEM_HEIGHT = 30;
inline constexpr int LIST_TILE_SIZE = 22;
inline constexpr int LIST_PAD = 6;
inline constexpr int PICKER_PAD = 10;
inline constexpr int PICKER_SUB_HEADER_HEIGHT = 28;
// The drawn cell size the picker's grid aims for.
inline constexpr int PICKER_TILE_TARGET = 64;

// The height of the panels between the header and the hint bar. Input and
// drawing both read it, so a click and the row drawn under it come from one
// row count.
//
// Example:
//   body_height(900);   // -> 824, after a 48 header and a 28 hint bar
[[nodiscard]] constexpr int body_height(int screenHeight)
{
	return screenHeight - HEADER_HEIGHT - HINT_HEIGHT;
}
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
	void handle_picker(const EditorInput& input, const Renderer& renderer);

	void render(const EditorInput& input, const GameContext& ctx) const;
	void render_header(const Renderer& renderer) const;
	void render_list(const EditorInput& input, const GameContext& ctx) const;
	void render_fields(
		const EditorInput& input,
		const Renderer& renderer) const;
	void render_picker(
		const EditorInput& input,
		const Renderer& renderer) const;
	void render_hint(const Renderer& renderer) const;

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
