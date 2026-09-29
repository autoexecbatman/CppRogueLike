// file: FovMapTest.cpp
// The field of view grid: what a creature standing somewhere can see.
//
// FovMap is recursive shadowcasting and had no test. What it promises: the cell
// you stand on is always seen; a cell is seen only if it lies within the radius,
// measured as a circle rather than a square; an opaque cell is seen but nothing
// behind it is; and each computation replaces the last rather than adding to it.
//
// The radius is a disc because the implementation compares dx*dx + dy*dy against
// radius*radius, so a cell straight ahead at the full radius is lit while one
// diagonally at the full radius is not - that pair is what tells a circle from a
// square, and it is the one case a square-radius mistake would pass.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=FovMapTest.*

#include <gtest/gtest.h>

#include "src/FovMap.h"

namespace
{
constexpr int GRID_SIDE = 40;
constexpr int STANDING_AT = 20;
constexpr int SIGHT = 5;
} // namespace

class FovMapTest : public ::testing::Test
{
protected:
	// Every cell starts opaque and unwalkable, so a test opens what it needs.
	void open_everything()
	{
		for (int row = 0; row < GRID_SIDE; ++row)
		{
			for (int col = 0; col < GRID_SIDE; ++col)
			{
				fov.set_properties(col, row, true, true);
			}
		}
	}

	void wall_at(int col, int row)
	{
		fov.set_properties(col, row, false, false);
	}

	void look_from_the_middle()
	{
		fov.compute_fov(STANDING_AT, STANDING_AT, SIGHT);
	}

	FovMap fov{ GRID_SIDE, GRID_SIDE };
};

// Whatever else is true, you can see where you are - even standing in a wall.
TEST_F(FovMapTest, TheCellYouStandOnIsAlwaysSeen)
{
	look_from_the_middle();
	EXPECT_TRUE(fov.is_in_fov(STANDING_AT, STANDING_AT)) << "standing in the dark and blind to it";

	open_everything();
	look_from_the_middle();
	EXPECT_TRUE(fov.is_in_fov(STANDING_AT, STANDING_AT));
}

// The pair that tells a circle from a square: straight ahead at the full radius
// is inside it, diagonally at the full radius is not.
TEST_F(FovMapTest, SightReachesInACircleRatherThanASquare)
{
	open_everything();
	look_from_the_middle();

	EXPECT_TRUE(fov.is_in_fov(STANDING_AT + SIGHT, STANDING_AT)) << "the full radius straight ahead is dark";
	EXPECT_FALSE(fov.is_in_fov(STANDING_AT + SIGHT, STANDING_AT + SIGHT))
		<< "the corner of the square is lit, so the radius is not a circle";
}

// Nothing past sight, however open the ground.
TEST_F(FovMapTest, NothingBeyondTheRadiusIsSeen)
{
	open_everything();
	look_from_the_middle();

	EXPECT_FALSE(fov.is_in_fov(STANDING_AT + SIGHT + 1, STANDING_AT));
	EXPECT_FALSE(fov.is_in_fov(STANDING_AT, STANDING_AT - SIGHT - 1));
}

// The point of a field of view: the wall is seen, the room behind it is not.
TEST_F(FovMapTest, AWallIsSeenAndWhatItHidesIsNot)
{
	open_everything();
	wall_at(STANDING_AT + 1, STANDING_AT);
	look_from_the_middle();

	EXPECT_TRUE(fov.is_in_fov(STANDING_AT + 1, STANDING_AT)) << "the wall itself should be seen";
	EXPECT_FALSE(fov.is_in_fov(STANDING_AT + 2, STANDING_AT)) << "the cell behind the wall is lit through it";
	EXPECT_FALSE(fov.is_in_fov(STANDING_AT + 3, STANDING_AT));
}

// Each look replaces the last. A view that accumulated would leave a creature
// seeing rooms it walked out of.
TEST_F(FovMapTest, LookingAgainFromElsewhereForgetsTheFirstView)
{
	open_everything();
	look_from_the_middle();
	ASSERT_TRUE(fov.is_in_fov(STANDING_AT, STANDING_AT));

	const int farAway = STANDING_AT + 3 * SIGHT;
	fov.compute_fov(farAway, farAway, SIGHT);

	EXPECT_FALSE(fov.is_in_fov(STANDING_AT, STANDING_AT)) << "the old view is still lit";
	EXPECT_TRUE(fov.is_in_fov(farAway, farAway));
}

// Asking about a cell off the grid answers no rather than reading past the end.
TEST_F(FovMapTest, AskingOffTheGridAnswersNo)
{
	open_everything();
	look_from_the_middle();

	EXPECT_FALSE(fov.is_in_fov(-1, 0));
	EXPECT_FALSE(fov.is_in_fov(0, -1));
	EXPECT_FALSE(fov.is_in_fov(GRID_SIDE, 0));
	EXPECT_FALSE(fov.is_walkable(GRID_SIDE, GRID_SIDE));
}

// Walkability is stored and answered back; it is separate from transparency,
// which is what lets a window be seen through and not walked into.
TEST_F(FovMapTest, WalkableAndTransparentAreRememberedApart)
{
	fov.set_properties(3, 4, true, false);
	EXPECT_TRUE(fov.is_walkable(3, 4));

	fov.set_properties(5, 6, false, true);
	EXPECT_FALSE(fov.is_walkable(5, 6));

	fov.compute_fov(5, 6, SIGHT);
	EXPECT_TRUE(fov.is_in_fov(5, 6)) << "an unwalkable cell can still be seen";
}

// end of file: FovMapTest.cpp
