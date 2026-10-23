// file: CostOverlayTest.cpp
// The debug cost overlay: what a tile says about its cost, and the colour it is drawn in.
// Expected tints are worked from the two end colours by the blend the header states, so a
// change to the blend fails here even where the result still looks plausible on screen.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CostOverlayTest.*

#include <gtest/gtest.h>

#include <raylib.h>

#include "src/CostOverlay.h"

namespace
{

void expect_tint(Color actual, Color expected)
{
	EXPECT_EQ(actual.r, expected.r);
	EXPECT_EQ(actual.g, expected.g);
	EXPECT_EQ(actual.b, expected.b);
	EXPECT_EQ(actual.a, expected.a);
}

} // namespace

TEST(CostOverlayTest, StartsHiddenAndEachToggleFlipsIt)
{
	CostOverlay overlay{};
	EXPECT_FALSE(overlay.is_visible());

	overlay.toggle();
	EXPECT_TRUE(overlay.is_visible());

	overlay.toggle();
	EXPECT_FALSE(overlay.is_visible());
}

TEST(CostOverlayTest, AReachedTileIsLabelledWithItsCost)
{
	EXPECT_EQ(dijkstra_cost_label(0), std::optional<std::string>{ "0" });
	EXPECT_EQ(dijkstra_cost_label(17), std::optional<std::string>{ "17" });
}

TEST(CostOverlayTest, ATileTheFloodNeverReachedHasNoLabel)
{
	EXPECT_EQ(dijkstra_cost_label(std::nullopt), std::nullopt);
}

TEST(CostOverlayTest, ThePlayerTileIsNearAndTheFarthestTileIsFar)
{
	expect_tint(dijkstra_cost_tint(0, 40), COST_NEAR_TINT);
	expect_tint(dijkstra_cost_tint(40, 40), COST_FAR_TINT);
}

// Halfway is the mean of the two ends on every channel: (230+40)/2, (70+110)/2, (40+230)/2.
TEST(CostOverlayTest, AHalfwayTileIsTintedHalfwayBetween)
{
	expect_tint(dijkstra_cost_tint(20, 40), Color{ 135, 90, 135, 120 });
}

// Only the player's own tile reached: the far end is cost 0 too, and must not divide by it.
TEST(CostOverlayTest, WithOnlyThePlayerTileReachedItIsNear)
{
	expect_tint(dijkstra_cost_tint(0, 0), COST_NEAR_TINT);
}
