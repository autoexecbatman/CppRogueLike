// file: TileConfig.h
#pragma once

#include <string>
#include <unordered_map>

#include "TileDefinition.h"
#include "TileType.h"
#include "Renderer.h"

// ---------------------------------------------------------------------------
// Autotile groups (3x3 blocks, origin = top-left corner of the group)
// ---------------------------------------------------------------------------
struct AutotileGroup
{
	TileSheet sheet{};
	int origin_col{};
	int origin_row{};
};

struct TileOffset
{
	int col{};
	int row{};
};

struct WallAutotileGroup
{
	TileSheet sheet{};
	int origin_col{};
	int origin_row{};
};

// ---------------------------------------------------------------------------
// Autotile -- resolve functions live in a namespace, not the global scope.
// ---------------------------------------------------------------------------
namespace Autotile
{
	constexpr TileRef resolve(AutotileGroup group, bool north, bool east, bool south, bool west)
	{
		int col_offset = (!west && east) ? 0 : (west && !east) ? 2 : 1;
		int row_offset = (!north && south) ? 0 : (north && !south) ? 2 : 1;
		return TileRef{ group.sheet, group.origin_col + col_offset, group.origin_row + row_offset };
	}

	constexpr TileRef resolve_mask(AutotileGroup group, int mask)
	{
		return resolve(
			group,
			(mask & 8) != 0,
			(mask & 4) != 0,
			(mask & 2) != 0,
			(mask & 1) != 0);
	}

	// Wall autotile (6-column DawnLike format)
	inline constexpr TileOffset WALL_TABLE[16] = {
		{ 3, 0 }, //  0: ....  Isolated pillar
		{ 1, 0 }, //  1: ...W  Horizontal (endcap fallback)
		{ 0, 1 }, //  2: ..S.  Vertical (endcap fallback)
		{ 2, 0 }, //  3: ..SW  Corner TR
		{ 1, 0 }, //  4: .E..  Horizontal (endcap fallback)
		{ 1, 0 }, //  5: .E.W  Horizontal
		{ 0, 0 }, //  6: .ES.  Corner TL
		{ 4, 0 }, //  7: .ESW  T-junction top
		{ 0, 1 }, //  8: N...  Vertical (endcap fallback)
		{ 2, 2 }, //  9: N..W  Corner BR
		{ 0, 1 }, // 10: N.S.  Vertical
		{ 5, 1 }, // 11: N.SW  T-junction right
		{ 0, 2 }, // 12: NE..  Corner BL
		{ 4, 2 }, // 13: NE.W  T-junction bottom
		{ 3, 1 }, // 14: NES.  T-junction left
		{ 4, 1 }, // 15: NESW  Center (fully surrounded)
	};

	constexpr TileRef wall_resolve(WallAutotileGroup group, bool north, bool east, bool south, bool west)
	{
		int mask = (north ? 8 : 0) | (east ? 4 : 0) | (south ? 2 : 0) | (west ? 1 : 0);
		auto offset = WALL_TABLE[mask];
		return TileRef{ group.sheet, group.origin_col + offset.col, group.origin_row + offset.row };
	}

	constexpr TileRef wall_resolve_mask(WallAutotileGroup group, int mask)
	{
		auto offset = WALL_TABLE[mask & 0xF];
		return TileRef{ group.sheet, group.origin_col + offset.col, group.origin_row + offset.row };
	}
} // namespace Autotile

// ---------------------------------------------------------------------------
// TileConfig -- runtime registry for system tile IDs loaded from JSON.
//
// The god editor authors data/tiles/tile_config.json; this class consumes it.
// load() must be called before any code accesses tiles.
// ---------------------------------------------------------------------------
class TileConfig
{
public:
	[[nodiscard]] TileRef get(std::string_view key) const;
	[[nodiscard]] AutotileGroup get_autotile(std::string_view key) const;
	[[nodiscard]] WallAutotileGroup get_wall_autotile(std::string_view key) const;

	// What a tile type does: whether it blocks, what bypasses it, what it says,
	// and how it draws on the minimap. Throws when the type has no entry, since
	// a tile the game can place and cannot describe is a data error.
	//
	// Example:
	//   const TileDefinition& water = config.get_tile_definition(TileType::WATER);
	//   water.blocksMovement;  // -> true
	//   water.bypassState;     // -> ActorState::CAN_SWIM
	[[nodiscard]] const TileDefinition& get_tile_definition(TileType tileType) const;
	void load(std::string_view path);

private:
	std::unordered_map<std::string, TileRef> m_tiles;
	std::unordered_map<std::string, AutotileGroup> m_autotile_groups;
	std::unordered_map<std::string, WallAutotileGroup> m_wall_autotile_groups;
	std::unordered_map<TileType, TileDefinition> m_tile_definitions;
};

// end of file: TileConfig.h
