#pragma once
// FovMap.h -- standalone FOV grid replacing libtcod TCODMap.

#include <vector>

// ---------------------------------------------------------------------------
// FovCell -- per-cell flags used by FovMap.
// ---------------------------------------------------------------------------
struct FovCell
{
	bool walkable{ false };
	bool transparent{ false };
	bool visible{ false };
};

// ---------------------------------------------------------------------------
// FovMap -- replaces TCODMap.
//
// Stores per-cell walkability/transparency and computes FOV using
// recursive shadowcasting (Bjorn Pettersen's algorithm).
// ---------------------------------------------------------------------------
class FovMap
{
public:
	FovMap(int width, int height);

	void set_properties(int x, int y, bool walkable, bool transparent) noexcept;
	bool is_walkable(int x, int y) const noexcept;
	bool is_in_fov(int x, int y) const noexcept;
	void compute_fov(int ox, int oy, int radius);

private:
	int width_;
	int height_;
	std::vector<FovCell> cells_;

	bool in_bounds(int x, int y) const noexcept;
	int cell_index(int x, int y) const noexcept;

	void scan_octant(
		int cx,
		int cy,
		int radius,
		int row,
		float start,
		float end,
		int xx,
		int xy,
		int yx,
		int yy);
};
