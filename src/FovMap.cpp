// FovMap.cpp -- standalone FOV grid using recursive shadowcasting.
#include "FovMap.h"

FovMap::FovMap(int gridWidth, int gridHeight)
	: width(gridWidth), height(gridHeight), cells(gridWidth * gridHeight)
{
}

void FovMap::set_properties(int col, int row, FovProperties properties) noexcept
{
	if (!in_bounds(col, row))
	{
		return;
	}

	FovCell& cell = cells[cell_index(col, row)];
	cell.walkable = properties.walkable;
	cell.transparent = properties.transparent;
}

bool FovMap::is_walkable(int col, int row) const noexcept
{
	if (!in_bounds(col, row))
	{
		return false;
	}

	return cells[cell_index(col, row)].walkable;
}

bool FovMap::is_in_fov(int col, int row) const noexcept
{
	if (!in_bounds(col, row))
	{
		return false;
	}

	return cells[cell_index(col, row)].visible;
}

bool FovMap::in_bounds(int col, int row) const noexcept
{
	return col >= 0 && col < width && row >= 0 && row < height;
}

int FovMap::cell_index(int col, int row) const noexcept
{
	return row * width + col;
}

// ---------------------------------------------------------------------------
// FOV -- recursive shadowcasting
//
// Reference: Bjorn Pettersen's algorithm (Roguebasin C++ shadowcasting).
// For each of 8 octants, scan_octant() casts shadows row by row.
//   octantTransforms[4][8]: each column is one octant's four coefficients, in the order
//   scan_octant takes them.
// ---------------------------------------------------------------------------

void FovMap::compute_fov(int col, int row, int radius)
{
	for (auto& cell : cells)
	{
		cell.visible = false;
	}

	if (!in_bounds(col, row))
	{
		return;
	}

	cells[cell_index(col, row)].visible = true;

	static const int octantTransforms[4][8] = {
		{ 1, 0, 0, -1, -1, 0, 0, 1 },
		{ 0, 1, -1, 0, 0, -1, 1, 0 },
		{ 0, 1, 1, 0, 0, -1, -1, 0 },
		{ 1, 0, 0, 1, -1, 0, 0, -1 },
	};

	for (int octant = 0; octant < 8; ++octant)
	{
		scan_octant(
			col,
			row,
			radius,
			1,
			1.0f,
			0.0f,
			octantTransforms[0][octant],
			octantTransforms[1][octant],
			octantTransforms[2][octant],
			octantTransforms[3][octant]);
	}
}

void FovMap::scan_octant(
	int originCol,
	int originRow,
	int radius,
	int firstDepth,
	float start,
	float end,
	int colPerColOffset,
	int colPerRowOffset,
	int rowPerColOffset,
	int rowPerRowOffset)
{
	if (start < end)
	{
		return;
	}

	float nextStart = start;

	for (int depth = firstDepth; depth <= radius; ++depth)
	{
		bool blocked = false;
		const int rowOffset = -depth;

		for (int colOffset = -depth; colOffset <= 0; ++colOffset)
		{
			float leftSlope = (static_cast<float>(colOffset) - 0.5f) / (static_cast<float>(rowOffset) + 0.5f);
			float rightSlope = (static_cast<float>(colOffset) + 0.5f) / (static_cast<float>(rowOffset) - 0.5f);

			if (start < rightSlope)
			{
				continue;
			}

			if (end > leftSlope)
			{
				break;
			}

			const int col = originCol + colOffset * colPerColOffset + rowOffset * colPerRowOffset;
			const int row = originRow + colOffset * rowPerColOffset + rowOffset * rowPerRowOffset;

			if (in_bounds(col, row) && colOffset * colOffset + rowOffset * rowOffset <= radius * radius)
			{
				cells[cell_index(col, row)].visible = true;
			}

			if (blocked)
			{
				if (!in_bounds(col, row) || !cells[cell_index(col, row)].transparent)
				{
					nextStart = rightSlope;
				}
				else
				{
					blocked = false;
					start = nextStart;
				}
			}
			else if (!in_bounds(col, row) || !cells[cell_index(col, row)].transparent)
			{
				blocked = true;
				if (depth < radius)
				{
					scan_octant(originCol, originRow, radius, depth + 1, start, leftSlope, colPerColOffset, colPerRowOffset, rowPerColOffset, rowPerRowOffset);
				}
				nextStart = rightSlope;
			}
		}

		if (blocked)
		{
			break;
		}
	}
}
