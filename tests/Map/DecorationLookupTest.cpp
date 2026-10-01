// file: DecorationLookupTest.cpp
//
// What is standing on a tile, as far as decorations are concerned.
//
// Map::find_decoration_at answers "is there a barrel here" for the bump-to-break
// path and for the mouse cursor. Three things decide its answer: the tile, whether
// the decoration has already been broken, and whether there is a decoration list at
// all - a level built without one answers no rather than crashing.
//
// A broken decoration is still in the list. It is skipped rather than removed, so
// "not found" and "found but broken" are the same answer here and the sweep that
// erases them happens in the game loop.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DecorationLookupTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/Decoration.h"
#include "src/Map.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr Vector2D THE_TILE{ 5, 5 };
constexpr Vector2D ANOTHER_TILE{ 9, 9 };
} // namespace

class DecorationLookupTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.decorations = &decorations;
	}

	// Puts a decoration on a tile and hands it back, so a test can break it.
	Decoration& standing_at(Vector2D where)
	{
		auto decoration = std::make_unique<Decoration>();
		decoration->position = where;
		decoration->name = "barrel";
		Decoration* placed = decoration.get();
		decorations.push_back(std::move(decoration));
		return *placed;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	Map map{ 20, 20 };
};

TEST_F(DecorationLookupTest, ADecorationOnTheTileIsFound)
{
	Decoration& barrel = standing_at(THE_TILE);

	EXPECT_EQ(map.find_decoration_at(THE_TILE, ctx), &barrel);
}

TEST_F(DecorationLookupTest, ADecorationOnAnotherTileIsNotFound)
{
	standing_at(ANOTHER_TILE);

	EXPECT_EQ(map.find_decoration_at(THE_TILE, ctx), nullptr) << "a decoration was found on a tile it is not on";
}

// A broken decoration stays in the list until the loop sweeps it, so the lookup has
// to skip it rather than report a barrel that is no longer there.
TEST_F(DecorationLookupTest, ABrokenDecorationIsNotFound)
{
	Decoration& barrel = standing_at(THE_TILE);
	barrel.isBroken = true;

	EXPECT_EQ(map.find_decoration_at(THE_TILE, ctx), nullptr) << "a broken decoration was reported as standing";
}

// The first unbroken one wins, so a broken barrel does not hide a whole one beneath it.
TEST_F(DecorationLookupTest, ABrokenOneDoesNotHideAWholeOneOnTheSameTile)
{
	Decoration& broken = standing_at(THE_TILE);
	broken.isBroken = true;
	Decoration& whole = standing_at(THE_TILE);

	EXPECT_EQ(map.find_decoration_at(THE_TILE, ctx), &whole);
}

// A level built without a decoration list answers no rather than reaching through null.
TEST_F(DecorationLookupTest, ALevelWithNoDecorationListAnswersNothing)
{
	ctx.decorations = nullptr;

	EXPECT_EQ(map.find_decoration_at(THE_TILE, ctx), nullptr);
}
