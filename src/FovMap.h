#pragma once
// file: FovMap.h
//
// The field-of-view grid: for each cell of a map, whether a body can walk into it and
// whether sight passes across it, and which cells the last look saw.
//
// Map owns one and keeps it in step with its tiles: set_tile writes a cell's properties
// from the tile type, and a load rebuilds them all. Sight is recursive shadowcasting
// (Bjorn Pettersen's algorithm), with the radius measured as a circle.
//
// Usage, on a 40 by 40 grid opened everywhere but one wall:
//
//   FovMap grid{ 40, 40 };   // every cell starts solid and opaque
//   // ... set_properties(col, row, { .walkable = true, .transparent = true }) on each cell
//   grid.set_properties(21, 20, FovProperties{ .walkable = false, .transparent = false });
//   grid.compute_fov(20, 20, 5);   // look from (20,20), five cells far
//   grid.is_in_fov(21, 20);   // -> true, the wall itself is seen
//   grid.is_in_fov(22, 20);   // -> false, it lies behind the wall
//   grid.is_walkable(21, 20);   // -> false
//
// A position off the grid is a legal question: is_walkable and is_in_fov answer false,
// and set_properties ignores it, because callers ask about cells beyond a map's edge.

#include <cstddef>
#include <vector>

// One cell: what it lets through, and whether the last look saw it.
struct FovCell
{
	// A body can step into the cell.
	bool walkable{ false };
	// Sight passes across the cell to what lies behind it.
	bool transparent{ false };
	// The last compute_fov lit the cell.
	bool visible{ false };
};

// What a cell lets through: a body walking into it, and sight passing across it.
struct FovProperties
{
	// A body can step into the cell.
	bool walkable{ false };
	// Sight passes across the cell.
	bool transparent{ false };
};

// One map's cells: what each lets through, and which the last look saw.
class FovMap
{
private:
	// How one octant's offsets from the origin map onto the grid: col and row of a scanned
	// cell are the origin's plus each offset times the coefficient named for it.
	struct OctantTransform
	{
		// The column offset's share of the cell's column.
		int colPerColOffset{ 0 };
		// The row offset's share of the cell's column.
		int colPerRowOffset{ 0 };
		// The column offset's share of the cell's row.
		int rowPerColOffset{ 0 };
		// The row offset's share of the cell's row.
		int rowPerRowOffset{ 0 };
	};

	// Signed, as the col and row they bound are: callers ask about cells off the grid,
	// and in_bounds compares against these before any index is formed.
	int width{ 0 };
	// Rows, signed for the same reason.
	int height{ 0 };
	// Row-major, width cells to a row.
	std::vector<FovCell> cells{};

	// Whether (col, row) lies on the grid.
	//
	// Example, on a 40 by 40 grid:
	//   in_bounds(39, 0);   // -> true
	//   in_bounds(-1, 0);   // -> false
	bool in_bounds(int col, int row) const noexcept;

	// Where (col, row) sits in cells. The caller has checked in_bounds; this asserts it.
	//
	// Example, on a 40 by 40 grid:
	//   cell_index(3, 2);   // -> 83
	std::size_t cell_index(int col, int row) const noexcept;

	// Lights the cells of one octant from firstDepth outward, recursing past each opaque
	// cell with the slopes it leaves visible. compute_fov calls it once per octant.
	//
	// Example, the first octant from (20,20), five far, over the whole slope range:
	//   scan_octant(20, 20, 5, 1, 1.0f, 0.0f, OctantTransform{ 1, 0, 0, 1 });
	void scan_octant(
		int originCol,
		int originRow,
		int radius,
		int firstDepth,
		float start,
		float end,
		const OctantTransform& transform);

public:
	// A grid of the given size, every cell solid, opaque and unseen. Zero is a legal side.
	//
	// Example:
	//   FovMap grid{ 40, 40 };
	//   grid.is_walkable(5, 5);   // -> false until set_properties opens it
	FovMap(int gridWidth, int gridHeight);

	// Sets what (col, row) lets through. A position off the grid is ignored.
	//
	// Example:
	//   grid.set_properties(3, 4, FovProperties{ .walkable = true, .transparent = false });
	//   grid.is_walkable(3, 4);   // -> true
	//   grid.set_properties(-1, 0, FovProperties{ .walkable = true, .transparent = true });   // nothing
	void set_properties(int col, int row, FovProperties properties) noexcept;

	// Whether a body can step into (col, row); false off the grid.
	//
	// Example:
	//   grid.is_walkable(5, 6);   // -> false, set walkable = false
	//   grid.is_walkable(40, 40);   // -> false, off a 40 by 40 grid
	bool is_walkable(int col, int row) const noexcept;

	// Whether the last compute_fov lit (col, row); false off the grid.
	//
	// Example, after compute_fov(20, 20, 5) on an open grid:
	//   grid.is_in_fov(25, 20);   // -> true, the full radius straight ahead
	//   grid.is_in_fov(25, 25);   // -> false, the radius is a circle
	//   grid.is_in_fov(-1, 0);   // -> false
	bool is_in_fov(int col, int row) const noexcept;

	// Replaces the last view with what (col, row) sees within radius. The cell looked
	// from is always lit, even inside a wall; a look from off the grid lights nothing.
	//
	// Example, on an open grid:
	//   grid.compute_fov(20, 20, 5);
	//   grid.is_in_fov(20, 20);   // -> true
	//   grid.compute_fov(35, 35, 5);
	//   grid.is_in_fov(20, 20);   // -> false, the first view is gone
	void compute_fov(int col, int row, int radius);
};
