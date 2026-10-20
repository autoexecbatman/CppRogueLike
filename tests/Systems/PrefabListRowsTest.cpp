// file: PrefabListRowsTest.cpp
//
// PrefabListRows' contract: the room editor's saved-prefab list draws row s at
// panelTop + (font + 4) + 4 + s * (font + 4), below a one-line header and above a guide
// box of eight lines at the same pitch plus 8 pixels, and a click selects the row drawn
// under it. Every expected value is worked from those draw formulas with font 16, panel
// top 64 and panel height 600: row height 20, first row at 88, guide box from
// 664 - 168 = 496, and (496 - 88) / 20 = 20 whole rows.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=PrefabListRowsTest.*

#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

#include "src/RoomEditor.h"

namespace
{
const PrefabListRows rows{ 64, 600, 16 };
}

// Rows sit below the header line, one font height plus four apart.
TEST(PrefabListRowsTest, RowsAreDrawnBelowTheHeaderAtTheDrawnPitch)
{
	EXPECT_EQ(rows.row_height(), 20);
	EXPECT_EQ(rows.row_top(0), 88);
	EXPECT_EQ(rows.row_top(3), 148);
}

// The list stops at the guide box, and its last whole row ends at or above the box.
TEST(PrefabListRowsTest, TheListEndsAtTheGuideBox)
{
	EXPECT_EQ(rows.guide_top(), 496);
	EXPECT_EQ(rows.visible_rows(), 20u);
	EXPECT_LE(rows.row_top(rows.visible_rows() - 1) + rows.row_height(), rows.guide_top());
}

// The first and last pixel of every drawn row select that row.
TEST(PrefabListRowsTest, EveryPixelOfADrawnRowSelectsThatRow)
{
	for (size_t slot = 0; slot < 20; ++slot)
	{
		EXPECT_EQ(rows.slot_at(88 + static_cast<int>(slot) * 20), std::optional<size_t>{ slot }) << "slot " << slot;
		EXPECT_EQ(rows.slot_at(107 + static_cast<int>(slot) * 20), std::optional<size_t>{ slot }) << "slot " << slot;
	}
}

// The header line above the first row selects nothing.
TEST(PrefabListRowsTest, TheHeaderSelectsNothing)
{
	EXPECT_EQ(rows.slot_at(64), std::nullopt);
	EXPECT_EQ(rows.slot_at(87), std::nullopt);
}

// The strip below the last whole row, and the guide box, select nothing.
TEST(PrefabListRowsTest, BelowTheLastWholeRowSelectsNothing)
{
	EXPECT_EQ(rows.slot_at(488), std::nullopt);
	EXPECT_EQ(rows.slot_at(495), std::nullopt);
	EXPECT_EQ(rows.slot_at(500), std::nullopt);
	EXPECT_EQ(rows.slot_at(663), std::nullopt);
}
