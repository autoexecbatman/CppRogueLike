#pragma once
#include <string>
#include <vector>

// file: DungeonRoom.h
//
// Single source of truth for a BSP-generated room's geometry.
// All coordinates use the project convention: x = column, y = row.
//
// "floor" region  -- the walkable interior, col..col_end() x row..row_end()
// "wall"  ring    -- one cell outside on all four sides

enum class RoomType
{
	ENTRANCE,
	STANDARD,
	DANGER,
	TREASURE
};

struct DungeonRoom
{
	int col{ 0 }; // leftmost floor column
	int row{ 0 }; // topmost  floor row
	int width{ 0 }; // floor column count
	int height{ 0 }; // floor row    count
	RoomType type{ RoomType::STANDARD };
	std::string prefab_name; // name of the prefab that defines this room's layout
	std::vector<int> adjacentRoomIndices; // indices of rooms connected via corridors

	// Floor bounds (inclusive)
	[[nodiscard]] int col_end() const { return col + width - 1; }
	[[nodiscard]] int row_end() const { return row + height - 1; }

	// Surrounding wall positions
	[[nodiscard]] int left_wall() const { return col - 1; }
	[[nodiscard]] int right_wall() const { return col + width; }
	[[nodiscard]] int top_wall() const { return row - 1; }
	[[nodiscard]] int bottom_wall() const { return row + height; }

	// Geometric centre (used for corridor midpoints)
	[[nodiscard]] int center_col() const { return col + width / 2; }
	[[nodiscard]] int center_row() const { return row + height / 2; }

	// Total tile count including the wall ring
	[[nodiscard]] int total_width() const { return width + 2; }
	[[nodiscard]] int total_height() const { return height + 2; }

	// True if (c, r) lies inside the floor area
	[[nodiscard]] bool contains(int c, int r) const
	{
		return c >= col && c <= col_end() && r >= row && r <= row_end();
	}
};
