// file: ListCursor.cpp
//
// ListCursor and step_within; the contract and a usage example are in ListCursor.h.

#include "ListCursor.h"

#include <algorithm>
#include <cassert>

size_t step_within(
	size_t value,
	int steps,
	size_t highest)
{
	assert(value <= highest && "step_within: value is above its own bound");
	if (steps >= 0)
	{
		// Compared against the room left rather than added first, so the sum cannot wrap.
		const size_t magnitude = static_cast<size_t>(steps);
		return magnitude >= highest - value ? highest : value + magnitude;
	}
	// -(steps + 1) is at most INT_MAX, so INT_MIN's magnitude is taken without negating it.
	const size_t magnitude = static_cast<size_t>(-(steps + 1)) + 1;
	return magnitude >= value ? 0 : value - magnitude;
}

size_t ListCursor::max_first_visible(size_t visibleRows) const
{
	return rowCount > visibleRows ? rowCount - visibleRows : 0;
}

bool ListCursor::is_empty() const
{
	return rowCount == 0;
}

size_t ListCursor::current_row() const
{
	assert(!is_empty() && "current_row: the list is empty");
	return current;
}

bool ListCursor::can_move_up() const
{
	return current > 0;
}

bool ListCursor::can_move_down() const
{
	return current + 1 < rowCount;
}

size_t ListCursor::first_visible(size_t visibleRows) const
{
	return std::min(firstVisible, max_first_visible(visibleRows));
}

std::optional<size_t> ListCursor::row_under(
	size_t visibleRowOffset,
	size_t visibleRows) const
{
	const size_t row = first_visible(visibleRows) + visibleRowOffset;
	if (row >= rowCount)
	{
		return std::nullopt;
	}
	return row;
}

void ListCursor::move_up()
{
	assert(can_move_up() && "move_up: the cursor cannot move up");
	--current;
	if (current < firstVisible)
	{
		firstVisible = current;
	}
}

void ListCursor::move_down(size_t visibleRows)
{
	assert(can_move_down() && "move_down: the cursor cannot move down");
	assert(visibleRows > 0 && "move_down: no row is visible");
	// Start from the screen as drawn, so a stale scroll position cannot leave the cursor off it.
	firstVisible = first_visible(visibleRows);
	++current;
	if (current >= firstVisible + visibleRows)
	{
		firstVisible = current - visibleRows + 1;
	}
}

void ListCursor::select(size_t row)
{
	assert(row < rowCount && "select: the row is past the end of the list");
	current = row;
}

void ListCursor::scroll_by(
	int steps,
	size_t visibleRows)
{
	// Start from the screen as drawn, which is what makes a notch back from the bottom move at once.
	firstVisible = step_within(first_visible(visibleRows), steps, max_first_visible(visibleRows));
}

void ListCursor::set_row_count(size_t rows)
{
	rowCount = rows;
	const size_t lastRow = rows == 0 ? 0 : rows - 1;
	current = std::min(current, lastRow);
	firstVisible = std::min(firstVisible, lastRow);
}

void ListCursor::reset(size_t rows)
{
	rowCount = rows;
	current = 0;
	firstVisible = 0;
}
