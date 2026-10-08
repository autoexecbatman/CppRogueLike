// file: InventoryTabsTest.cpp
//
// Reaching the Backpack and Usables panes, by pointer and by key.
//
// What it is for. The tab bar could not be clicked at all. The drawing put it on TAB_ROW,
// which is 0, and the click test asked whether the pointer was on row 1 - a number
// written into the hit test rather than read from the constant the drawing uses. The two
// also walked the tab positions separately, each computing its own x, so they agreed
// about across and disagreed about down.
//
// They read one layout now. Clicking Equipment appeared to work only because the screen
// opens on it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=InventoryTabsTest.*

#include <gtest/gtest.h>

#include "src/InventoryUI.h"
#include "src/Renderer.h"

class InventoryTabsTest : public ::testing::Test
{
protected:
	// The vertical middle of the tab row, as the drawing computes its top.
	int tab_row_y() const
	{
		return panel_text_row_y(0, renderer.get_tile_size(), TAB_ROW) + 2;
	}

	int middle_of(const InventoryActions::TabBox& box) const
	{
		return box.x + box.width / 2;
	}

	Renderer renderer{};
};

TEST_F(InventoryTabsTest, TheThreeTabsRunLeftToRightWithoutOverlapping)
{
	const auto boxes = InventoryActions::tab_boxes(renderer);

	EXPECT_EQ(boxes[0].screen, InventoryScreen::EQUIPMENT);
	EXPECT_EQ(boxes[1].screen, InventoryScreen::BACKPACK);
	EXPECT_EQ(boxes[2].screen, InventoryScreen::USABLES);
	EXPECT_LT(boxes[0].x, boxes[1].x);
	EXPECT_LT(boxes[1].x, boxes[2].x);
}

TEST_F(InventoryTabsTest, EveryTabIsReachableByPointer)
{
	// The defect: this passed for Equipment by accident and failed for the other two,
	// because no tab was reachable and the screen already starts on Equipment.
	for (const auto& box : InventoryActions::tab_boxes(renderer))
	{
		const auto hit = InventoryActions::tab_at(middle_of(box), tab_row_y(), renderer);

		ASSERT_TRUE(hit.has_value()) << "a tab could not be clicked at its own middle";
		EXPECT_EQ(*hit, box.screen);
	}
}

TEST_F(InventoryTabsTest, APointerOffTheTabRowHitsNoTab)
{
	const auto boxes = InventoryActions::tab_boxes(renderer);
	const int contentRowY = panel_text_row_y(0, renderer.get_tile_size(), FIRST_CONTENT_ROW) + 2;

	EXPECT_FALSE(InventoryActions::tab_at(middle_of(boxes[1]), contentRowY, renderer).has_value())
		<< "a click on a slot row switched panes";
}

TEST_F(InventoryTabsTest, APointerPastTheLastTabHitsNoTab)
{
	const auto boxes = InventoryActions::tab_boxes(renderer);
	const int pastTheEnd = boxes[2].x + boxes[2].width + 200;

	EXPECT_FALSE(InventoryActions::tab_at(pastTheEnd, tab_row_y(), renderer).has_value());
}

TEST_F(InventoryTabsTest, TabStepsThroughEveryPaneAndComesBack)
{
	// The keyboard route. Three steps return to where they started, so no pane is
	// skipped and none is a dead end.
	InventoryScreen screen = InventoryScreen::EQUIPMENT;

	screen = InventoryActions::next_screen(screen);
	EXPECT_EQ(screen, InventoryScreen::BACKPACK);
	screen = InventoryActions::next_screen(screen);
	EXPECT_EQ(screen, InventoryScreen::USABLES);
	screen = InventoryActions::next_screen(screen);
	EXPECT_EQ(screen, InventoryScreen::EQUIPMENT);
}
