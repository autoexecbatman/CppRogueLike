// file: PanelRowGeometryTest.cpp
//
// The two directions of one map: a panel text row to its pixel, and a pixel back
// to its row.
//
// Every full-screen panel and menu lays its text out on UI_TEXT_ROW_PITCH rather
// than on the tile grid, because a tile grows with the zoom and a row of text does
// not. panel_text_row_y is the forward direction and has been right for a long
// time. The inverse is what every mouse hit test needs, and until 2026-10-01 three
// menus each derived it separately and two of them got it wrong - they divided the
// pointer's y by the tile size while the rows were drawn a pitch apart, so the row
// under the pointer and the row the menu selected drifted apart the further down
// the menu the pointer went.
//
// The property below is what makes that impossible to reintroduce: the two
// functions are inverses, at every zoom, for every row.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PanelRowGeometryTest.*

#include <gtest/gtest.h>

#include "src/Renderer.h"

namespace
{
// The zooms the renderer actually offers, smallest to largest.
constexpr int TILE_SIZES[] = { 16, 32, 48, 64, 96, 128 };
constexpr int PANEL_TOPS[] = { 0, 64, 192 };
} // namespace

// The property: a pixel taken from a row maps back to that row.
TEST(PanelRowGeometryTest, ARowsTopPixelMapsBackToThatRow)
{
	for (int tileSize : TILE_SIZES)
	{
		for (int panelTop : PANEL_TOPS)
		{
			for (int row = 0; row < 12; ++row)
			{
				const int topPixel = panel_text_row_y(panelTop, tileSize, row);
				EXPECT_EQ(panel_text_row_at_y(panelTop, tileSize, topPixel), row)
					<< "tile " << tileSize << ", panel top " << panelTop << ", row " << row;
			}
		}
	}
}

// And so does every pixel inside it, which is what a pointer actually lands on.
TEST(PanelRowGeometryTest, EveryPixelInsideARowMapsToThatRow)
{
	for (int tileSize : TILE_SIZES)
	{
		for (int row = 0; row < 12; ++row)
		{
			const int topPixel = panel_text_row_y(0, tileSize, row);
			for (int offset = 0; offset < UI_TEXT_ROW_PITCH; ++offset)
			{
				EXPECT_EQ(panel_text_row_at_y(0, tileSize, topPixel + offset), row)
					<< "tile " << tileSize << ", row " << row << ", " << offset << " pixels down";
			}
		}
	}
}

// Above the first row the answer is negative, so a caller can tell "not in the
// list" from "the first entry". Truncating division reports 0 for the pixel just
// above row 0, which is the whole of this test.
TEST(PanelRowGeometryTest, APixelAboveTheFirstRowIsNegative)
{
	for (int tileSize : TILE_SIZES)
	{
		const int firstRowTop = panel_text_row_y(0, tileSize, 0);

		EXPECT_LT(panel_text_row_at_y(0, tileSize, firstRowTop - 1), 0)
			<< "the pixel one above the first row read as being inside it, at tile " << tileSize;
		EXPECT_LT(panel_text_row_at_y(0, tileSize, 0), 0)
			<< "the top of the panel frame read as a text row, at tile " << tileSize;
	}
}

// The rows are a pitch apart and the pitch does not change with the zoom. This is
// the claim the broken hit tests contradicted: they treated a row as one tile tall.
TEST(PanelRowGeometryTest, RowsAreAPitchApartRegardlessOfZoom)
{
	for (int tileSize : TILE_SIZES)
	{
		EXPECT_EQ(
			panel_text_row_y(0, tileSize, 4) - panel_text_row_y(0, tileSize, 3),
			UI_TEXT_ROW_PITCH)
			<< "row spacing moved with the tile size, at tile " << tileSize;
	}
}
