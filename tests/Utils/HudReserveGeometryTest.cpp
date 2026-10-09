// file: HudReserveGeometryTest.cpp
//
// What the HUD reserves at the bottom of the screen, against what it then draws
// inside that reservation.
//
// The panel is framed on all four sides and its text sits between the two
// horizontal rules. Those rules are art: a frame tile is 64 pixels and paints only
// part of itself, so the pixels a panel must clear are a fraction of the tile and
// not the whole of it. Reserving a whole tile for each rule silently costs the
// panel two rows of text at the shipping zoom, and reserving too little puts the
// last row under the bottom rule where it cannot be read.
//
// The property below is what makes either impossible to reintroduce: whatever
// gui_reserve_rows hands back has to be tall enough for every row GUI_TEXT_ROWS
// claims, at every zoom.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HudReserveGeometryTest.*

#include <gtest/gtest.h>

#include "src/Renderer.h"

namespace
{
// The zooms the renderer actually offers, smallest to largest.
constexpr int TILE_SIZES[] = { 16, 32, 48, 64, 96, 128 };

// The font is loaded once at a fixed size and does not follow the zoom, so these
// are sizes the HUD has shipped at rather than a function of the tile.
constexpr int FONT_SIZES[] = { 14, 16, 30 };

// Bottom of the last row of text the HUD lays out, measured from the panel's top.
int last_text_row_bottom(int tileSize, int fontSize)
{
	return gui_frame_top_rule(tileSize) + GUI_TEXT_TOP_INSET + (GUI_TEXT_ROWS - 1) * UI_TEXT_ROW_PITCH + fontSize;
}
} // namespace

// The property: every row the HUD claims to lay out lands above the bottom rule.
TEST(HudReserveGeometryTest, EveryTextRowClearsTheBottomRule)
{
	for (int tileSize : TILE_SIZES)
	{
		for (int fontSize : FONT_SIZES)
		{
			const int reservedPixels = gui_reserve_rows(tileSize, fontSize) * tileSize;
			const int firstPixelOfBottomRule = reservedPixels - gui_frame_bottom_rule(tileSize);

			EXPECT_LE(last_text_row_bottom(tileSize, fontSize), firstPixelOfBottomRule)
				<< "the last of " << GUI_TEXT_ROWS << " rows ran under the bottom rule, at tile "
				<< tileSize << " and font " << fontSize;
		}
	}
}

// And the reservation is a ceiling rather than merely enough: one tile row fewer has
// to be too few. Without this the test above passes on a panel that reserves half the
// screen, since a reservation that is too large satisfies every row.
TEST(HudReserveGeometryTest, OneTileRowFewerWouldNotFit)
{
	for (int tileSize : TILE_SIZES)
	{
		for (int fontSize : FONT_SIZES)
		{
			const int neededPixels = last_text_row_bottom(tileSize, fontSize) + gui_frame_bottom_rule(tileSize);
			const int oneRowFewer = (gui_reserve_rows(tileSize, fontSize) - 1) * tileSize;

			EXPECT_LT(oneRowFewer, neededPixels)
				<< "the HUD reserved a tile row it does not need, at tile " << tileSize
				<< " and font " << fontSize;
		}
	}
}

// The rule depths are read off DawnLike/GUI/Frame.png, whose cell is 64 pixels: the
// top edge paints rows 1 to 28 and the bottom edge rows 33 to 60. A frame drawn at
// another tile size scales, so the depths have to scale with it.
TEST(HudReserveGeometryTest, RuleDepthsMatchTheArtAndScaleWithTheTile)
{
	EXPECT_EQ(gui_frame_top_rule(64), 29);
	EXPECT_EQ(gui_frame_bottom_rule(64), 31);

	EXPECT_EQ(gui_frame_top_rule(32), 14) << "the top rule did not halve with the tile";
	EXPECT_EQ(gui_frame_bottom_rule(32), 15) << "the bottom rule did not halve with the tile";
	EXPECT_EQ(gui_frame_top_rule(128), 58) << "the top rule did not double with the tile";
}

// The vertical rules are the same art turned on its side: the left edge paints columns
// 1 to 29 of its tile and a divider columns 17 to 46, so both are 30 wide. Anything
// placed against a rule clears this, and a value from the old hairline frame puts the
// first glyph of every line on top of the brass.
TEST(HudReserveGeometryTest, TheRuleIsAsWideAsTheArtPaintsIt)
{
	EXPECT_EQ(gui_frame_rule_width(64), 30);

	EXPECT_EQ(gui_frame_rule_width(32), 15) << "the rule did not halve with the tile";
	EXPECT_EQ(gui_frame_rule_width(128), 60) << "the rule did not double with the tile";

	EXPECT_GT(gui_frame_rule_width(64), 8)
		<< "the rule is back to the 8 pixels the hairline frame drew, which no longer "
		   "covers what the art paints";
}

// A rule is a fraction of its tile, so reserving a whole tile for each one wastes
// the rest. This pins the size the game ships at: five tile rows hold eight rows of
// text, where reserving two whole tiles holds six.
TEST(HudReserveGeometryTest, TheShippingZoomReservesFiveRowsForEightLinesOfText)
{
	EXPECT_EQ(gui_reserve_rows(64, 30), 5);
	EXPECT_EQ(GUI_TEXT_ROWS, 8);

	const int wholeTileReserve = 64 + GUI_TEXT_TOP_INSET + (GUI_TEXT_ROWS - 1) * UI_TEXT_ROW_PITCH + 30 + 64;
	EXPECT_GT(wholeTileReserve, 5 * 64)
		<< "a whole tile an edge still fits, so this test no longer measures anything";
}
