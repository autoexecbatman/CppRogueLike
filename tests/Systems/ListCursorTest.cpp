// file: ListCursorTest.cpp
//
// ListCursor's contract: an editor list's current row and first drawn row, with no
// wrap and no out-of-range state. Every expected value comes from the contract in
// ListCursor.h, not from any editor's current behaviour, and each one is chosen so
// that a cursor answering 0, false or nullopt everywhere fails it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=ListCursorTest.*:StepWithinTest.*

#include <climits>
#include <optional>

#include <gtest/gtest.h>

#include "src/ListCursor.h"

// An empty list answers no on every move and keeps both positions at 0.
TEST(ListCursorTest, AnEmptyListHasNoMovesAndStaysAtZero)
{
	ListCursor cursor{};
	EXPECT_TRUE(cursor.is_empty());
	EXPECT_FALSE(cursor.can_move_up());
	EXPECT_FALSE(cursor.can_move_down());
	EXPECT_EQ(cursor.first_visible(5), 0u);
	cursor.scroll_by(-3, 5);
	EXPECT_EQ(cursor.first_visible(5), 0u);
	cursor.scroll_by(3, 5);
	EXPECT_EQ(cursor.first_visible(5), 0u);
	cursor.set_row_count(0);
	EXPECT_TRUE(cursor.is_empty());
}

// One row: it is current, nothing moves, and only the top drawn place holds a row.
TEST(ListCursorTest, OneRowCannotMoveAndOnlyItsOwnPlaceHoldsARow)
{
	ListCursor cursor{};
	cursor.reset(1);
	EXPECT_FALSE(cursor.is_empty());
	EXPECT_FALSE(cursor.can_move_up());
	EXPECT_FALSE(cursor.can_move_down());
	EXPECT_EQ(cursor.first_visible(5), 0u);
	EXPECT_EQ(cursor.row_under(0, 5), std::optional<size_t>{ 0 });
	EXPECT_EQ(cursor.row_under(4, 5), std::nullopt);
}

// Down past the bottom of a three-row screen scrolls so the cursor is the last drawn row.
TEST(ListCursorTest, DownPastTheBottomEdgeScrollsTheCursorIntoView)
{
	ListCursor cursor{};
	cursor.reset(10);
	ASSERT_TRUE(cursor.can_move_down());
	for (int step = 0; step < 5; ++step)
	{
		cursor.move_down(3);
	}
	EXPECT_EQ(cursor.current_row(), 5u);
	EXPECT_EQ(cursor.first_visible(3), 3u);
	EXPECT_TRUE(cursor.can_move_up());
}

// Up past the top of the screen scrolls so the cursor is the first drawn row.
TEST(ListCursorTest, UpPastTheTopEdgeScrollsTheCursorIntoView)
{
	ListCursor cursor{};
	cursor.reset(10);
	for (int step = 0; step < 5; ++step)
	{
		cursor.move_down(3);
	}
	ASSERT_EQ(cursor.first_visible(3), 3u);
	for (int step = 0; step < 3; ++step)
	{
		cursor.move_up();
	}
	EXPECT_EQ(cursor.current_row(), 2u);
	EXPECT_EQ(cursor.first_visible(3), 2u);
}

// The cursor stops at the last row: there is no down from it, and up is still open.
TEST(ListCursorTest, TheLastRowHasNoDown)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.select(9);
	EXPECT_FALSE(cursor.can_move_down());
	EXPECT_TRUE(cursor.can_move_up());
}

// The wheel stops at the last full screen and at the top, and does nothing when every row fits.
TEST(ListCursorTest, TheWheelStopsAtTheLastScreenfulAndAtTheTop)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.scroll_by(100, 3);
	EXPECT_EQ(cursor.first_visible(3), 7u);
	cursor.scroll_by(-1, 3);
	EXPECT_EQ(cursor.first_visible(3), 6u) << "one notch back from the bottom moves at once";
	cursor.scroll_by(-100, 3);
	EXPECT_EQ(cursor.first_visible(3), 0u);
	cursor.scroll_by(2, 20);
	EXPECT_EQ(cursor.first_visible(20), 0u);
	EXPECT_EQ(cursor.current_row(), 0u) << "the wheel moves the screen, not the cursor";
}

// Deleting the current last row leaves the row above it current.
TEST(ListCursorTest, DeletingTheLastRowSelectsTheRowAboveIt)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.select(9);
	cursor.set_row_count(9);
	EXPECT_EQ(cursor.current_row(), 8u);
	EXPECT_FALSE(cursor.can_move_down());
}

// A row count that still reaches the current row leaves it where it was.
TEST(ListCursorTest, AddingARowKeepsTheCurrentRow)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.select(4);
	cursor.set_row_count(11);
	EXPECT_EQ(cursor.current_row(), 4u);
}

// Shrinking to nothing empties the list; a reset after a shrink starts again at the top.
TEST(ListCursorTest, ShrinkingToEmptyAndResettingAfterAShrink)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.select(6);
	cursor.set_row_count(0);
	EXPECT_TRUE(cursor.is_empty());
	EXPECT_FALSE(cursor.can_move_down());

	cursor.reset(10);
	cursor.select(4);
	cursor.scroll_by(5, 3);
	cursor.reset(3);
	EXPECT_FALSE(cursor.is_empty());
	EXPECT_EQ(cursor.current_row(), 0u);
	EXPECT_EQ(cursor.first_visible(1), 0u);
	EXPECT_TRUE(cursor.can_move_down()) << "three rows, so row 0 has a row below it";
}

// A click maps a drawn place to its row, and a place below the last row to nothing.
TEST(ListCursorTest, RowUnderMapsADrawnPlaceAndRefusesOneBelowTheLastRow)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.scroll_by(7, 3);
	EXPECT_EQ(cursor.row_under(2, 3), std::optional<size_t>{ 9 });
	EXPECT_EQ(cursor.row_under(0, 3), std::optional<size_t>{ 7 });
	EXPECT_EQ(cursor.row_under(5, 3), std::nullopt);
}

// An empty list has no row under any place.
TEST(ListCursorTest, RowUnderOnAnEmptyListIsNothing)
{
	ListCursor cursor{};
	EXPECT_EQ(cursor.row_under(0, 3), std::nullopt);
}

// After a shrink, a click reads the row that is drawn there, not the stale scroll position.
TEST(ListCursorTest, RowUnderAfterAShrinkReadsTheRowThatIsDrawn)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.scroll_by(7, 3);
	cursor.set_row_count(5);
	ASSERT_EQ(cursor.first_visible(3), 2u);
	EXPECT_EQ(cursor.row_under(0, 3), std::optional<size_t>{ 2 });
	EXPECT_EQ(cursor.row_under(2, 3), std::optional<size_t>{ 4 });
	EXPECT_EQ(cursor.row_under(3, 3), std::nullopt);
}

// After a shrink, one notch of the wheel moves the screen as drawn at once.
TEST(ListCursorTest, TheWheelAfterAShrinkMovesTheDrawnScreenAtOnce)
{
	ListCursor cursor{};
	cursor.reset(10);
	cursor.scroll_by(7, 3);
	cursor.set_row_count(5);
	ASSERT_EQ(cursor.first_visible(3), 2u);
	cursor.scroll_by(-1, 3);
	EXPECT_EQ(cursor.first_visible(3), 1u);
}

// Emptying the list and filling it again starts the screen at the top, where the cursor is.
TEST(ListCursorTest, EmptyingAndRefillingStartsTheScreenAtTheTop)
{
	ListCursor cursor{};
	cursor.reset(4);
	cursor.scroll_by(1, 3);
	ASSERT_EQ(cursor.first_visible(3), 1u);
	cursor.set_row_count(0);
	cursor.set_row_count(4);
	EXPECT_EQ(cursor.current_row(), 0u);
	EXPECT_EQ(cursor.first_visible(3), 0u);
}

// A step down starts from the screen as drawn, so a scroll position left above it by a
// shrink is gone once the list grows again.
TEST(ListCursorTest, AStepDownAfterAShrinkKeepsTheDrawnScreen)
{
	ListCursor cursor{};
	cursor.reset(4);
	cursor.scroll_by(1, 3);
	cursor.set_row_count(2);
	cursor.move_down(3);
	cursor.set_row_count(4);
	EXPECT_EQ(cursor.current_row(), 1u);
	EXPECT_EQ(cursor.first_visible(3), 0u);
}

// A signed step clamps at zero and at the bound, and leaves the value alone at zero steps.
TEST(StepWithinTest, AStepIsHeldBetweenZeroAndTheBound)
{
	EXPECT_EQ(step_within(2, -5, 9), 0u);
	EXPECT_EQ(step_within(2, 5, 9), 7u);
	EXPECT_EQ(step_within(8, 5, 9), 9u);
	EXPECT_EQ(step_within(0, -1, 9), 0u);
	EXPECT_EQ(step_within(4, 0, 9), 4u);
	EXPECT_EQ(step_within(0, 3, 0), 0u);
	EXPECT_EQ(step_within(5, -2, 9), 3u);
}

// The extremes of int neither wrap nor overflow.
TEST(StepWithinTest, TheExtremesOfIntAreClampedWithoutOverflow)
{
	EXPECT_EQ(step_within(3, INT_MIN, 9), 0u);
	EXPECT_EQ(step_within(3, INT_MAX, 9), 9u);
}
