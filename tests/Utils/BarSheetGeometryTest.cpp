// file: BarSheetGeometryTest.cpp
// Holds the HUD's bar art and the constant that describes it to the same shape. The
// game draws a bar's trough between its two end plates and takes the width of a plate
// from GUI_BAR_PLATE_WIDTH; nothing in a build, a suite or a render at full health can
// tell that the number and the sheet have drifted apart.
//
// The other half is the composition: the rim is stretched over the fill, so wherever
// the rim is clear the fill has to paint, or a bar shows a hole through itself.

#include <gtest/gtest.h>
#include <raylib.h>

#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include "src/Paths.h"
#include "src/Renderer.h"

namespace
{
// One cell of the sheet, as which of its columns and rows paint anything. Loaded off
// disk rather than through the renderer, which would want a window and a graphics
// device for a question about pixels.
class BarSheet
{
private:
	Image image{};

	[[nodiscard]] bool is_painted(int cellCol, int cellRow, int col, int row) const
	{
		const Color pixel = GetImageColor(
			image, cellCol * GUI_BARS_CELL_SIZE + col, cellRow * GUI_BARS_CELL_SIZE + row);
		return pixel.a > 127;
	}

public:
	BarSheet()
	{
		const std::string path = Paths::resolve("DawnLike/GUI/Bars.png").string();
		image = LoadImage(path.c_str());
	}

	~BarSheet()
	{
		if (image.data != nullptr)
		{
			UnloadImage(image);
		}
	}

	BarSheet(const BarSheet&) = delete;
	BarSheet& operator=(const BarSheet&) = delete;
	BarSheet(BarSheet&&) = delete;
	BarSheet& operator=(BarSheet&&) = delete;

	[[nodiscard]] bool is_loaded() const { return image.data != nullptr; }

	[[nodiscard]] int width() const { return image.width; }

	[[nodiscard]] int height() const { return image.height; }

	// Left to right: whether each column of the cell paints anything at all.
	[[nodiscard]] std::vector<bool> painted_columns(int cellCol, int cellRow) const
	{
		std::vector<bool> columns(GUI_BARS_CELL_SIZE, false);
		for (int col = 0; col < GUI_BARS_CELL_SIZE; ++col)
		{
			for (int row = 0; row < GUI_BARS_CELL_SIZE; ++row)
			{
				if (is_painted(cellCol, cellRow, col, row))
				{
					columns[col] = true;
					break;
				}
			}
		}
		return columns;
	}

	// Top to bottom: whether each row of the cell paints anything at all.
	[[nodiscard]] std::vector<bool> painted_rows(int cellCol, int cellRow) const
	{
		std::vector<bool> rows(GUI_BARS_CELL_SIZE, false);
		for (int row = 0; row < GUI_BARS_CELL_SIZE; ++row)
		{
			for (int col = 0; col < GUI_BARS_CELL_SIZE; ++col)
			{
				if (is_painted(cellCol, cellRow, col, row))
				{
					rows[row] = true;
					break;
				}
			}
		}
		return rows;
	}
};

// Where each piece sits on the sheet, as data/tiles/tile_config.json addresses them.
constexpr int RIM_COL = 0;
constexpr int RIM_ROW = 0;
constexpr int FILL_COL = 1;
constexpr int FILL_ROW = 0;
constexpr int CAP_LEFT_COL = 0;
constexpr int CAP_LEFT_ROW = 2;
constexpr int CAP_RIGHT_COL = 1;
constexpr int CAP_RIGHT_ROW = 2;

int count_painted(const std::vector<bool>& flags)
{
	return static_cast<int>(std::count(flags.begin(), flags.end(), true));
}
}

TEST(BarSheetGeometryTest, TheSheetIsTwoCellsAcrossAndThreeDown)
{
	const BarSheet sheet;
	ASSERT_TRUE(sheet.is_loaded()) << "DawnLike/GUI/Bars.png did not load";
	EXPECT_EQ(sheet.width(), 2 * GUI_BARS_CELL_SIZE);
	EXPECT_EQ(sheet.height(), 3 * GUI_BARS_CELL_SIZE);
}

TEST(BarSheetGeometryTest, EachEndPlateIsAsWideAsTheTroughIsInset)
{
	const BarSheet sheet;
	ASSERT_TRUE(sheet.is_loaded()) << "DawnLike/GUI/Bars.png did not load";

	// A plate wider than the inset covers fill the inset was meant to clear, which is
	// how a bar with a few percent left draws as an empty one.
	EXPECT_EQ(count_painted(sheet.painted_columns(CAP_LEFT_COL, CAP_LEFT_ROW)), GUI_BAR_PLATE_WIDTH);
	EXPECT_EQ(count_painted(sheet.painted_columns(CAP_RIGHT_COL, CAP_RIGHT_ROW)), GUI_BAR_PLATE_WIDTH);
}

TEST(BarSheetGeometryTest, EachEndPlateSitsAgainstItsOwnEndOfTheCell)
{
	const BarSheet sheet;
	ASSERT_TRUE(sheet.is_loaded()) << "DawnLike/GUI/Bars.png did not load";

	// The two cells are drawn at the two ends of the bar at their own size, so the rest
	// of each cell has to be clear or a plate stands over the trough beside it.
	const std::vector<bool> leftPlate = sheet.painted_columns(CAP_LEFT_COL, CAP_LEFT_ROW);
	const std::vector<bool> rightPlate = sheet.painted_columns(CAP_RIGHT_COL, CAP_RIGHT_ROW);
	EXPECT_TRUE(leftPlate.front());
	EXPECT_FALSE(leftPlate.back());
	EXPECT_FALSE(rightPlate.front());
	EXPECT_TRUE(rightPlate.back());
}

TEST(BarSheetGeometryTest, TheFillPaintsEveryRowTheRimLeavesClearBetweenItsRails)
{
	const BarSheet sheet;
	ASSERT_TRUE(sheet.is_loaded()) << "DawnLike/GUI/Bars.png did not load";

	const std::vector<bool> rimRows = sheet.painted_rows(RIM_COL, RIM_ROW);
	const std::vector<bool> fillRows = sheet.painted_rows(FILL_COL, FILL_ROW);

	const auto topRail = std::find(rimRows.begin(), rimRows.end(), true);
	const auto bottomRail = std::find(rimRows.rbegin(), rimRows.rend(), true);
	ASSERT_NE(topRail, rimRows.end()) << "the rim paints nothing at all";

	// Between the rails is the trough, and it is clear so the fill drawn under the rim
	// shows through it. Every one of those rows has to be painted in the fill, or a bar
	// has a hole through it at that height.
	const int firstInterior = static_cast<int>(std::distance(rimRows.begin(), topRail)) + 1;
	const int lastInterior = GUI_BARS_CELL_SIZE - 1 - static_cast<int>(std::distance(rimRows.rbegin(), bottomRail)) - 1;

	int clearInteriorRows = 0;
	for (int row = firstInterior; row <= lastInterior; ++row)
	{
		if (!rimRows[row])
		{
			++clearInteriorRows;
			EXPECT_TRUE(fillRows[row]) << std::format("the rim is clear at row {} and the fill paints nothing there", row);
		}
	}
	EXPECT_GT(clearInteriorRows, 0) << "the rim is opaque from rail to rail, so no fill can show";
}
