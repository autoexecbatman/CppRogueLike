#pragma once

// file: ListCursor.h
//
// Which row of a list is current, and which row is drawn first, for an editor's
// scrolling list or field panel. A value type with no drawing, input or registry in it:
// the editor owns the rows and the pixels, and asks this one object every question
// about position in them, so no editor clamps a row or a scroll offset of its own.
//
// Rows are size_t throughout and nothing here can wrap: an empty list has no current
// row, the cursor cannot step past either end, and the wheel stops at the last
// screenful rather than scrolling until one row shows.
//
// Usage:
//
//   ListCursor cursor{};
//   cursor.reset(10);                  // ten rows, the first one current
//   cursor.move_down(3);               // three rows fit on screen -> current_row() 1
//   cursor.scroll_by(100, 3);          // the wheel, a signed count -> first_visible(3) 7
//   cursor.row_under(2, 3);            // a click on the third drawn row -> 9
//   cursor.row_under(5, 3);            // a click below the last row -> nullopt
//   cursor.set_row_count(0);           // every row deleted -> is_empty() true
//
// The editor asks a query before it issues the matching command - can_move_down()
// before move_down() - because it has to commit the row it is leaving first. Each
// command asserts what its query answered, so calling one without asking is caught.
//
// step_within is the one home for "a size_t moved by a signed step, clamped": the
// cursor's wheel is written on it, and so is any editor wheel that has no cursor.

#include <cstddef>
#include <optional>

// `value` moved by `steps` (negative steps move toward zero) and held to [0, highest],
// with no wrap in either direction. A step of INT_MIN is safe: its magnitude is taken
// without negating it. `value` must already be within its bound.
//
// Example:
//   step_within(2, -5, 9);   // -> 0, stopped at zero instead of wrapping
//   step_within(2, 5, 9);    // -> 7
//   step_within(8, 5, 9);    // -> 9, stopped at the bound
//   step_within(10, 1, 9);   // aborts: "step_within: value is above its own bound"
[[nodiscard]] size_t step_within(
	size_t value,
	int steps,
	size_t highest);

// How many whole rows of `rowHeight` pixels fit in `pixels`: 0 when not even one fits,
// including a negative extent. It is also the row a pixel falls in, counted from 0, for a
// pixel already known to be at or below the list's top. This is where a pixel extent from
// raylib, an int, becomes a row count.
//
// Example:
//   whole_rows_within(89, 30);   // -> 2, the third row is cut off
//   whole_rows_within(-5, 30);   // -> 0
//   whole_rows_within(90, 0);    // aborts: "whole_rows_within: a row has no height"
[[nodiscard]] size_t whole_rows_within(
	int pixels,
	int rowHeight);

class ListCursor
{
private:
	size_t rowCount{ 0 };
	size_t current{ 0 };
	size_t firstVisible{ 0 };

	// The highest first row that still fills a screen of `visibleRows`, or 0 when every
	// row fits.
	//
	// Example:
	//   // 10 rows
	//   max_first_visible(3);    // -> 7, rows 7, 8 and 9 fill the last screen
	//   max_first_visible(20);   // -> 0, every row fits
	[[nodiscard]] size_t max_first_visible(size_t visibleRows) const;

public:
	// Whether the list has no rows, and so no current row.
	//
	// Example:
	//   ListCursor{}.is_empty();   // -> true
	//   // after reset(1)
	//   is_empty();                // -> false
	[[nodiscard]] bool is_empty() const;

	// The current row, the one index to read the editor's keys with. An empty list has
	// none, so asking is a caller that skipped is_empty().
	//
	// Example:
	//   // after reset(10) and select(4)
	//   current_row();             // -> 4
	//   ListCursor{}.current_row();   // aborts: "current_row: the list is empty"
	[[nodiscard]] size_t current_row() const;

	// Whether the cursor can step up one row: false on an empty list and at row 0.
	//
	// Example:
	//   // after reset(10)
	//   can_move_up();             // -> false, already at the first row
	//   // after select(4)
	//   can_move_up();             // -> true
	[[nodiscard]] bool can_move_up() const;

	// Whether the cursor can step down one row: false on an empty list and at the last row.
	//
	// Example:
	//   // after reset(10)
	//   can_move_down();           // -> true
	//   // after select(9)
	//   can_move_down();           // -> false, already at the last row
	[[nodiscard]] bool can_move_down() const;

	// The first row drawn on a screen of `visibleRows`: the stored scroll position, held
	// to the last full screen. The draw path reads this rather than clamping its own copy.
	//
	// Example:
	//   // 10 rows, after scroll_by(100, 3)
	//   first_visible(3);          // -> 7
	//   first_visible(20);         // -> 0, every row fits, so nothing scrolls
	[[nodiscard]] size_t first_visible(size_t visibleRows) const;

	// The row drawn `visibleRowOffset` rows below the top of a screen of `visibleRows`,
	// or nullopt when that place is past the bottom of the screen, below the last row, or
	// the list is empty. A click on empty space under the list selects nothing.
	//
	// Example:
	//   // 10 rows, after scroll_by(7, 3)
	//   row_under(2, 3);           // -> 9
	//   row_under(5, 3);           // -> nullopt, past the screen and below the last row
	[[nodiscard]] std::optional<size_t> row_under(
		size_t visibleRowOffset,
		size_t visibleRows) const;

	// Steps the cursor up one row, scrolling up when it leaves the top of the screen.
	// Requires can_move_up().
	//
	// Example:
	//   // 10 rows, current 5, first visible 3
	//   move_up(); move_up(); move_up();   // -> current_row() 2, first_visible(3) 2
	//   // at row 0
	//   move_up();                 // aborts: "move_up: the cursor cannot move up"
	void move_up();

	// Steps the cursor down one row, scrolling so it stays on a screen of `visibleRows`.
	// Requires can_move_down() and at least one visible row.
	//
	// Example:
	//   // 10 rows, current 0, 3 rows on screen
	//   move_down(3) five times;   // -> current_row() 5, first_visible(3) 3
	//   // at row 9
	//   move_down(3);              // aborts: "move_down: the cursor cannot move down"
	void move_down(size_t visibleRows);

	// Makes `row` current, as a click does; the scroll position is left where it is.
	//
	// Example:
	//   // 10 rows
	//   select(4);                 // -> current_row() 4
	//   select(10);                // aborts: "select: the row is past the end of the list"
	void select(size_t row);

	// Moves the scroll position by the wheel's signed count, held between the first row
	// and the last full screen of `visibleRows`. The cursor does not move.
	//
	// Example:
	//   // 10 rows
	//   scroll_by(100, 3);         // -> first_visible(3) 7
	//   scroll_by(-100, 3);        // -> first_visible(3) 0
	void scroll_by(
		int steps,
		size_t visibleRows);

	// Takes a new row count after an add or a delete, keeping the current row and the
	// scroll position where they were unless the list no longer reaches them.
	//
	// Example:
	//   // 10 rows, current 9
	//   set_row_count(9);          // -> current_row() 8, the row above the deleted one
	//   set_row_count(0);          // -> is_empty() true
	void set_row_count(size_t rows);

	// Starts a new list of `rows` rows, the first one current and drawn at the top, as a
	// tab switch does.
	//
	// Example:
	//   // 10 rows, current 4
	//   reset(3);                  // -> current_row() 0, first_visible(3) 0, three rows
	void reset(size_t rows);
};
