// file: FovMap.cpp
//
// The field-of-view grid's storage and its recursive shadowcasting. FovMap.h holds the
// usage and each function's contract.

#include <array>
#include <cassert>
#include <cstddef>

#include "FovMap.h"

FovMap::FovMap(int gridWidth, int gridHeight) :
	width(gridWidth),
	height(gridHeight)
{
	assert(
		gridWidth >= 0 && gridHeight >= 0 && "FovMap: a grid side is negative");
	cells.resize(
		static_cast<std::size_t>(gridWidth) *
		static_cast<std::size_t>(gridHeight));
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

std::size_t FovMap::cell_index(int col, int row) const noexcept
{
	assert(
		in_bounds(col, row) &&
		"FovMap::cell_index: the caller did not check in_bounds");
	return static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
		static_cast<std::size_t>(col);
}

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

	// The eight octants, each mapping the scan's offsets onto the grid.
	static constexpr std::array<OctantTransform, 8> OCTANT_TRANSFORMS{ {
		{ 1, 0, 0, 1 },
		{ 0, 1, 1, 0 },
		{ 0, -1, 1, 0 },
		{ -1, 0, 0, 1 },
		{ -1, 0, 0, -1 },
		{ 0, -1, -1, 0 },
		{ 0, 1, -1, 0 },
		{ 1, 0, 0, -1 },
	} };

	for (const OctantTransform& transform : OCTANT_TRANSFORMS)
	{
		scan_octant(col, row, radius, 1, 1.0f, 0.0f, transform);
	}
}

void FovMap::scan_octant(
	int originCol,
	int originRow,
	int radius,
	int firstDepth,
	float start,
	float end,
	const OctantTransform& transform)
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
			float leftSlope = (static_cast<float>(colOffset) - 0.5f) /
				(static_cast<float>(rowOffset) + 0.5f);
			float rightSlope = (static_cast<float>(colOffset) + 0.5f) /
				(static_cast<float>(rowOffset) - 0.5f);

			if (start < rightSlope)
			{
				continue;
			}

			if (end > leftSlope)
			{
				break;
			}

			const int col = originCol + colOffset * transform.colPerColOffset +
				rowOffset * transform.colPerRowOffset;
			const int row = originRow + colOffset * transform.rowPerColOffset +
				rowOffset * transform.rowPerRowOffset;

			if (in_bounds(col, row) &&
				colOffset * colOffset + rowOffset * rowOffset <=
					radius * radius)
			{
				cells[cell_index(col, row)].visible = true;
			}

			if (blocked)
			{
				if (!in_bounds(col, row) ||
					!cells[cell_index(col, row)].transparent)
				{
					nextStart = rightSlope;
				}
				else
				{
					blocked = false;
					start = nextStart;
				}
			}
			else if (
				!in_bounds(col, row) ||
				!cells[cell_index(col, row)].transparent)
			{
				blocked = true;
				if (depth < radius)
				{
					scan_octant(
						originCol,
						originRow,
						radius,
						depth + 1,
						start,
						leftSlope,
						transform);
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
