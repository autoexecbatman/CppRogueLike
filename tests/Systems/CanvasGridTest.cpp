// file: CanvasGridTest.cpp
//
// CanvasGrid's contract: the room editor draws canvas cell (col, row) with its top-left
// corner at (originX + col * tileSize, originY + row * tileSize), where the origin is
// the canvas area's corner less the pan, and a pixel belongs to the cell drawn over it.
// Expected values are worked from that rule with tile size 32: on x the area starts at
// 100 and the canvas is panned 40 right (origin 140); on y the area starts at 100 and
// the canvas is panned 40 the other way (origin 60).
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=CanvasGridTest.*

#include <gtest/gtest.h>

#include "src/RoomEditor.h"

namespace
{
const CanvasGrid grid{ 140, 60, 32 };
}

// Cells are drawn one tile apart from the origin.
TEST(CanvasGridTest, CellsAreDrawnOneTileApartFromTheOrigin)
{
	EXPECT_EQ(grid.cell_left(0), 140);
	EXPECT_EQ(grid.cell_left(3), 236);
	EXPECT_EQ(grid.cell_top(0), 60);
	EXPECT_EQ(grid.cell_top(2), 124);
}

// Both edge pixels of a drawn cell belong to that cell.
TEST(CanvasGridTest, BothEdgesOfADrawnCellBelongToIt)
{
	EXPECT_EQ(grid.column_at(140), 0);
	EXPECT_EQ(grid.column_at(171), 0);
	EXPECT_EQ(grid.column_at(172), 1);
	EXPECT_EQ(grid.row_at(91), 0);
	EXPECT_EQ(grid.row_at(92), 1);
	EXPECT_EQ(grid.row_at(100), 1);
}

// The strip left of column 0 is column -1, which no click paints.
TEST(CanvasGridTest, ThePixelsLeftOfColumnZeroAreColumnMinusOne)
{
	EXPECT_EQ(grid.column_at(139), -1);
	EXPECT_EQ(grid.column_at(110), -1);
	EXPECT_EQ(grid.column_at(108), -1);
}

// A pixel more than one tile left of the origin is column -2.
TEST(CanvasGridTest, AWholeTileLeftOfColumnMinusOneIsColumnMinusTwo)
{
	EXPECT_EQ(grid.column_at(107), -2);
}

// Above row 0 is row -1, on a grid panned so its origin is above the pixel's area.
TEST(CanvasGridTest, ThePixelsAboveRowZeroAreRowMinusOne)
{
	const CanvasGrid pannedDown{ 140, 140, 32 };
	EXPECT_EQ(pannedDown.row_at(139), -1);
	EXPECT_EQ(pannedDown.row_at(110), -1);
}
