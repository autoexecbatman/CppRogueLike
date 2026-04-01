// FovMap.cpp -- standalone FOV grid using recursive shadowcasting.
// Replaces libtcod TCODMap/TCODPath entirely.
#include "FovMap.h"

FovMap::FovMap(int width, int height)
	: width_(width), height_(height), cells_(width * height)
{
}

void FovMap::set_properties(int x, int y, bool walkable, bool transparent) noexcept
{
	if (!in_bounds(x, y))
	{
		return;
	}

	FovCell& c = cells_[cell_index(x, y)];
	c.walkable = walkable;
	c.transparent = transparent;
}

bool FovMap::is_walkable(int x, int y) const noexcept
{
	if (!in_bounds(x, y))
	{
		return false;
	}

	return cells_[cell_index(x, y)].walkable;
}

bool FovMap::is_in_fov(int x, int y) const noexcept
{
	if (!in_bounds(x, y))
	{
		return false;
	}

	return cells_[cell_index(x, y)].visible;
}

bool FovMap::in_bounds(int x, int y) const noexcept
{
	return x >= 0 && x < width_ && y >= 0 && y < height_;
}

int FovMap::cell_index(int x, int y) const noexcept
{
	return y * width_ + x;
}

// ---------------------------------------------------------------------------
// FOV -- recursive shadowcasting
//
// Reference: Bjorn Pettersen's algorithm (Roguebasin C++ shadowcasting).
// For each of 8 octants, scan_octant() casts shadows row by row.
//   mult[4][8]: each column is {xx, xy, yx, yy} for one octant.
// ---------------------------------------------------------------------------

void FovMap::compute_fov(int panelX, int panelY, int radius)
{
	for (auto& c : cells_)
	{
		c.visible = false;
	}

	if (!in_bounds(panelX, panelY))
	{
		return;
	}

	cells_[cell_index(panelX, panelY)].visible = true;

	static const int mult[4][8] = {
		{ 1, 0, 0, -1, -1, 0, 0, 1 },
		{ 0, 1, -1, 0, 0, -1, 1, 0 },
		{ 0, 1, 1, 0, 0, -1, -1, 0 },
		{ 1, 0, 0, 1, -1, 0, 0, -1 },
	};

	for (int oct = 0; oct < 8; ++oct)
	{
		scan_octant(
			panelX,
			panelY,
			radius,
			1,
			1.0f,
			0.0f,
			mult[0][oct],
			mult[1][oct],
			mult[2][oct],
			mult[3][oct]);
	}
}

void FovMap::scan_octant(
	int cx,
	int cy,
	int radius,
	int row,
	float start,
	float end,
	int xx,
	int xy,
	int yx,
	int yy)
{
	if (start < end)
	{
		return;
	}

	float new_start = start;

	for (int i = row; i <= radius; ++i)
	{
		bool blocked = false;
		const int dy = -i;

		for (int dx = -i; dx <= 0; ++dx)
		{
			float l_slope = (static_cast<float>(dx) - 0.5f) / (static_cast<float>(dy) + 0.5f);
			float r_slope = (static_cast<float>(dx) + 0.5f) / (static_cast<float>(dy) - 0.5f);

			if (start < r_slope)
			{
				continue;
			}

			if (end > l_slope)
			{
				break;
			}

			const int sx = cx + dx * xx + dy * xy;
			const int sy = cy + dx * yx + dy * yy;

			if (in_bounds(sx, sy) && dx * dx + dy * dy <= radius * radius)
			{
				cells_[cell_index(sx, sy)].visible = true;
			}

			if (blocked)
			{
				if (!in_bounds(sx, sy) || !cells_[cell_index(sx, sy)].transparent)
				{
					new_start = r_slope;
				}
				else
				{
					blocked = false;
					start = new_start;
				}
			}
			else if (!in_bounds(sx, sy) || !cells_[cell_index(sx, sy)].transparent)
			{
				blocked = true;
				if (i < radius)
				{
					scan_octant(cx, cy, radius, i + 1, start, l_slope, xx, xy, yx, yy);
				}
				new_start = r_slope;
			}
		}

		if (blocked)
		{
			break;
		}
	}
}
