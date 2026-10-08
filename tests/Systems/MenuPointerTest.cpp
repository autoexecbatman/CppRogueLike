// file: MenuPointerTest.cpp
//
// Where a click lands: turning a screen pixel into the menu row under it.
//
// What it is for. Three of twenty menus handled the mouse and eleven that write their
// own menu() handled none, so the shop, ability allocation and targeting were keyboard
// only. The handling that did exist lived inside ListMenu, so every other menu would
// have had to copy it.
//
// Lifting it also fixed what the copy would have spread: ListMenu resolved a click by
// its y alone. A text row's line runs the whole width of the window, so a click far to
// the side of a narrow menu landed on whatever row shared its height - and the comment
// beside it said "click outside menu bounds = ignore", which is what it did not do.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MenuPointerTest.*

#include <gtest/gtest.h>

#include "src/BaseMenu.h"
#include "src/InputSystem.h"
#include "src/Renderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// BaseMenu is abstract and menu_row_at is for derived menus, so the probe is one.
// Nothing here draws; the geometry is answered from the menu's own box.
class ProbeMenu : public BaseMenu
{
public:
	void menu(GameContext&) override {}

	std::optional<size_t> row_at(int pixelX, int pixelY, size_t rowCount) const
	{
		return menu_row_at(pixelX, pixelY, rowCount);
	}
};
} // namespace

class MenuPointerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.renderer = &renderer;
		ctx.inputSystem = &input;

		// Four tiles in, three down, eight wide and six tall.
		probe.menu_new(PANEL_TILES_WIDE, PANEL_TILES_TALL, PANEL_COL, PANEL_ROW, ctx);
		tileSize = renderer.get_tile_size();
	}

	// The vertical middle of a text row, as the drawing side computes it.
	int middle_of_row(int row) const
	{
		return panel_text_row_y(PANEL_ROW * tileSize, tileSize, row) + 2;
	}

	int inside_x() const { return (PANEL_COL * tileSize) + (tileSize / 2); }

	static constexpr int PANEL_COL = 4;
	static constexpr int PANEL_ROW = 3;
	static constexpr int PANEL_TILES_WIDE = 8;
	static constexpr int PANEL_TILES_TALL = 6;
	static constexpr size_t ROWS = 5;

	MockGameContext mock{};
	GameContext ctx{};
	Renderer renderer{};
	InputSystem input{};
	ProbeMenu probe{};
	int tileSize{ 0 };
};

TEST_F(MenuPointerTest, APointerOnARowAnswersThatRow)
{
	for (size_t row = 0; row < ROWS; ++row)
	{
		const auto answer = probe.row_at(inside_x(), middle_of_row(static_cast<int>(row)), ROWS);
		ASSERT_TRUE(answer.has_value()) << "row " << row << " was not reachable";
		EXPECT_EQ(*answer, row);
	}
}

TEST_F(MenuPointerTest, APointerAboveTheFirstRowIsOnTheFrame)
{
	EXPECT_FALSE(probe.row_at(inside_x(), PANEL_ROW * tileSize, ROWS).has_value());
}

TEST_F(MenuPointerTest, APointerBelowTheLastRowAnswersNothing)
{
	EXPECT_FALSE(probe.row_at(inside_x(), middle_of_row(static_cast<int>(ROWS)), ROWS).has_value());
}

TEST_F(MenuPointerTest, APointerRightOfThePanelAnswersNothing)
{
	// The defect being lifted out of ListMenu: a row's line runs the whole window, so a
	// click level with a row but well past the panel's right edge used to fire it.
	const int pastTheRightEdge = (PANEL_COL + PANEL_TILES_WIDE + 3) * tileSize;

	EXPECT_FALSE(probe.row_at(pastTheRightEdge, middle_of_row(1), ROWS).has_value())
		<< "a click " << pastTheRightEdge << " pixels across hit a menu that ends at "
		<< (PANEL_COL + PANEL_TILES_WIDE) * tileSize;
}

TEST_F(MenuPointerTest, APointerLeftOfThePanelAnswersNothing)
{
	EXPECT_FALSE(probe.row_at(0, middle_of_row(1), ROWS).has_value());
}
