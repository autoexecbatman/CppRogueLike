// file: TileConfig.h
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "../Renderer/Renderer.h"

// ---------------------------------------------------------------------------
// Autotile groups (3x3 blocks, origin = top-left corner of the group)
// ---------------------------------------------------------------------------
struct AutotileGroup
{
	TileSheet sheet;
	int origin_col;
	int origin_row;
};

constexpr TileRef autotile_resolve(AutotileGroup g, bool n, bool e, bool s, bool w)
{
	int col_offset = (!w && e) ? 0 : (w && !e) ? 2 : 1;
	int row_offset = (!n && s) ? 0 : (n && !s) ? 2 : 1;
	return TileRef{ g.sheet, g.origin_col + col_offset, g.origin_row + row_offset };
}

constexpr TileRef autotile_resolve_mask(AutotileGroup g, int mask)
{
	return autotile_resolve(
		g,
		(mask & 8) != 0,
		(mask & 4) != 0,
		(mask & 2) != 0,
		(mask & 1) != 0);
}

// ---------------------------------------------------------------------------
// Wall autotile (6-column DawnLike format)
// ---------------------------------------------------------------------------
struct TileOffset { int col; int row; };

inline constexpr TileOffset WALL_AUTOTILE_TABLE[16] = {
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

struct WallAutotileGroup
{
	TileSheet sheet;
	int origin_col;
	int origin_row;
};

constexpr TileRef wall_autotile_resolve(WallAutotileGroup g, bool n, bool e, bool s, bool w)
{
	int mask = (n ? 8 : 0) | (e ? 4 : 0) | (s ? 2 : 0) | (w ? 1 : 0);
	auto off = WALL_AUTOTILE_TABLE[mask];
	return TileRef{ g.sheet, g.origin_col + off.col, g.origin_row + off.row };
}

constexpr TileRef wall_autotile_resolve_mask(WallAutotileGroup g, int mask)
{
	auto off = WALL_AUTOTILE_TABLE[mask & 0xF];
	return TileRef{ g.sheet, g.origin_col + off.col, g.origin_row + off.row };
}

// ---------------------------------------------------------------------------
// TileConfig -- runtime registry for system tile IDs loaded from JSON.
//
// Replaces all former inline constexpr TILE_* / AUTOTILE_* / WALL_AUTOTILE_*
// / GUI_* / TILE_EFFECT_* constants. The god editor authors
// data/tiles/tile_config.json; this class consumes it.
// load() must be called before any code accesses tiles.
// ---------------------------------------------------------------------------
class TileConfig
{
public:
	[[nodiscard]] static TileConfig& instance();

	[[nodiscard]] TileRef get(std::string_view key) const;
	[[nodiscard]] AutotileGroup get_autotile(std::string_view key) const;
	[[nodiscard]] WallAutotileGroup get_wall_autotile(std::string_view key) const;

	void load(std::string_view path);

private:
	TileConfig() = default;

	std::unordered_map<std::string, TileRef> m_tiles;
	std::unordered_map<std::string, AutotileGroup> m_autotile_groups;
	std::unordered_map<std::string, WallAutotileGroup> m_wall_autotile_groups;
};

// end of file: TileConfig.h
