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

// What a cell lets through: a body walking into it, and sight passing across it.
struct FovProperties
{
	bool walkable{ false };
	bool transparent{ false };
};

// ---------------------------------------------------------------------------
// FovMap -- the field-of-view grid.
//
// Stores per-cell walkability/transparency and computes FOV using
// recursive shadowcasting (Bjorn Pettersen's algorithm).
// ---------------------------------------------------------------------------
class FovMap
{
private:
	int width{ 0 };
	int height{ 0 };
	std::vector<FovCell> cells{};

	bool in_bounds(int col, int row) const noexcept;
	int cell_index(int col, int row) const noexcept;

	void scan_octant(
		int originCol,
		int originRow,
		int radius,
		int firstDepth,
		float start,
		float end,
		int colPerColOffset,
		int colPerRowOffset,
		int rowPerColOffset,
		int rowPerRowOffset);

public:
	FovMap(int gridWidth, int gridHeight);

	void set_properties(int col, int row, FovProperties properties) noexcept;
	bool is_walkable(int col, int row) const noexcept;
	bool is_in_fov(int col, int row) const noexcept;
	void compute_fov(int col, int row, int radius);
};
