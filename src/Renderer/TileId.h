// file: TileId.h
#pragma once

// DawnLike sprite sheet indices.
// Sheets with 0/1 suffixes are animation frame pairs.
enum TileSheet : int
{
    SHEET_FLOOR = 0,
    SHEET_WALL,
    SHEET_DOOR0,
    SHEET_DOOR1,
    SHEET_PLAYER0,
    SHEET_PLAYER1,
    SHEET_HUMANOID0,
    SHEET_HUMANOID1,
    SHEET_REPTILE0,
    SHEET_REPTILE1,
    SHEET_PEST0,
    SHEET_PEST1,
    SHEET_DOG0,
    SHEET_DOG1,
    SHEET_AVIAN0,
    SHEET_AVIAN1,
    SHEET_UNDEAD0,
    SHEET_UNDEAD1,
    SHEET_QUADRAPED0,
    SHEET_QUADRAPED1,
    SHEET_DEMON0,
    SHEET_DEMON1,
    SHEET_MISC0,
    SHEET_MISC1,
    SHEET_POTION,
    SHEET_SCROLL,
    SHEET_SHORT_WEP,
    SHEET_MED_WEP,
    SHEET_LONG_WEP,
    SHEET_ARMOR,
    SHEET_SHIELD,
    SHEET_HAT,
    SHEET_RING,
    SHEET_AMULET_ITEM,
    SHEET_FOOD,
    SHEET_FLESH,
    SHEET_MONEY,
    SHEET_TILE,
    SHEET_DECOR0,
    SHEET_DECOR1,
    SHEET_EFFECT0,
    SHEET_EFFECT1,
    SHEET_PIT0,
    SHEET_GUI0,
    SHEET_GUI1,
    SHEET_COUNT // needs to be last, used for array size
};

// Encode a DawnLike tile reference as a single int.
// Layout: bits 23-16 = sheet, bits 15-8 = row, bits 7-0 = column.
constexpr int make_tile(int sheet, int col, int row)
{
    return (sheet << 16) | (row << 8) | col;
}

// Decode helpers
constexpr int tile_sheet(int tile_id) { return (tile_id >> 16) & 0xFF; }
constexpr int tile_row(int tile_id)   { return (tile_id >> 8) & 0xFF; }
constexpr int tile_col(int tile_id)   { return tile_id & 0xFF; }

// ---------------------------------------------------------------------------
// Map tiles
// ---------------------------------------------------------------------------
// Floor.png autotile groups are 3x3.  Center tile = (group_col+1, group_row+1).
// First stone group starts at row 3, cols 0-2.
inline constexpr int TILE_FLOOR_STONE = make_tile(SHEET_FLOOR, 1, 4);   // light stone center
inline constexpr int TILE_CORRIDOR    = make_tile(SHEET_FLOOR, 1, 7);   // medium stone center
inline constexpr int TILE_WATER       = make_tile(SHEET_PIT0, 1, 13);  // dark navy floor

// Primary Wall Tile -- matches the center/fully-surrounded autotile entry.
inline constexpr int TILE_WALL_STONE  = make_tile(SHEET_WALL, 4, 4);

inline constexpr int TILE_DOOR_CLOSED = make_tile(SHEET_DOOR0, 0, 0);   // solid closed door
inline constexpr int TILE_DOOR_OPEN   = make_tile(SHEET_DOOR0, 1, 0);   // open door frame
inline constexpr int TILE_STAIRS      = make_tile(SHEET_TILE, 5, 3);    // staircase

// ---------------------------------------------------------------------------
// Autotile groups (3x3 blocks, origin = top-left corner of the group)
// ---------------------------------------------------------------------------
struct AutotileGroup
{
    int sheet;
    int origin_col;
    int origin_row;
};

inline constexpr AutotileGroup AUTOTILE_FLOOR_STONE = { SHEET_FLOOR, 0, 3 };
inline constexpr AutotileGroup AUTOTILE_CORRIDOR    = { SHEET_FLOOR, 0, 6 };
// Wall autotile uses WallAutotileGroup (6-column format) below.

// Compute floor autotile variant from 4-neighbor presence flags.
// n/e/s/w = true when same-type tile exists in that direction.
// Works for 3-column autotile groups (floors).
constexpr int autotile_resolve(AutotileGroup group, bool n, bool e, bool s, bool w)
{
    int col_offset = (!w && e) ? 0 : (w && !e) ? 2 : 1;
    int row_offset = (!n && s) ? 0 : (n && !s) ? 2 : 1;
    return make_tile(group.sheet, group.origin_col + col_offset, group.origin_row + row_offset);
}

// Same logic but accepts a pre-built 4-bit NESW bitmask (N=8,E=4,S=2,W=1).
constexpr int autotile_resolve_mask(AutotileGroup group, int mask)
{
    bool n = (mask & 8) != 0;
    bool e = (mask & 4) != 0;
    bool s = (mask & 2) != 0;
    bool w = (mask & 1) != 0;
    return autotile_resolve(group, n, e, s, w);
}

// ---------------------------------------------------------------------------
// Wall autotile (6-column DawnLike format)
// ---------------------------------------------------------------------------
// DawnLike Wall.png uses 6 cols x 3 rows per wall type.
// First 3 rows of the sheet are guide/legend tiles.
//
// Grid Layout (verified from guide tile analysis):
// Row 0: TL(6),   Horz(5), TR(3),   Pillar,  T-Up(7),  Void
// Row 1: Vert(10), Pillar,  Void,    T-L(14), Cen(15),  T-R(11)
// Row 2: BL(12),   Void,    BR(9),   Void,    T-Dn(13), Void
//
// Masks 0,1,2,4,8 (endcaps) have no dedicated tiles in DawnLike.
// Fallback: pillar for isolated, horz for E/W-only, vert for N/S-only.

struct TileOffset
{
    int col;
    int row;
};

// Lookup table indexed by 4-bit NESW bitmask.
inline constexpr TileOffset WALL_AUTOTILE_TABLE[16] =
{
    {3, 0}, //  0: ....  Isolated pillar
    {1, 0}, //  1: ...W  Horizontal (endcap fallback)
    {0, 1}, //  2: ..S.  Vertical (endcap fallback)
    {2, 0}, //  3: ..SW  Corner TR
    {1, 0}, //  4: .E..  Horizontal (endcap fallback)
    {1, 0}, //  5: .E.W  Horizontal
    {0, 0}, //  6: .ES.  Corner TL
    {4, 0}, //  7: .ESW  T-junction top
    {0, 1}, //  8: N...  Vertical (endcap fallback)
    {2, 2}, //  9: N..W  Corner BR
    {0, 1}, // 10: N.S.  Vertical
    {5, 1}, // 11: N.SW  T-junction right
    {0, 2}, // 12: NE..  Corner BL
    {4, 2}, // 13: NE.W  T-junction bottom
    {3, 1}, // 14: NES.  T-junction left
    {4, 1}, // 15: NESW  Center (fully surrounded)
};

struct WallAutotileGroup
{
    int sheet;
    int origin_col;
    int origin_row;
};

// DawnLike Wall.png: Second group starts at row 3.
inline constexpr WallAutotileGroup WALL_AUTOTILE_STONE = { SHEET_WALL, 0, 3 };

// Returns tile_id for the wall.
constexpr int wall_autotile_resolve(WallAutotileGroup group, bool n, bool e, bool s, bool w)
{
    int mask = (n ? 8 : 0) | (e ? 4 : 0) | (s ? 2 : 0) | (w ? 1 : 0);
    auto offset = WALL_AUTOTILE_TABLE[mask];

    return make_tile(
        group.sheet,
        group.origin_col + offset.col,
        group.origin_row + offset.row
    );
}

constexpr int wall_autotile_resolve_mask(WallAutotileGroup group, int mask)
{
    // Direct lookup! No boolean logic needed.
    auto offset = WALL_AUTOTILE_TABLE[mask & 0xF]; 
    return make_tile(group.sheet, group.origin_col + offset.col, group.origin_row + offset.row);
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------
inline constexpr int TILE_PLAYER = make_tile(SHEET_PLAYER0, 1, 3);

// ---------------------------------------------------------------------------
// Monsters
// ---------------------------------------------------------------------------
inline constexpr int TILE_GOBLIN        = make_tile(SHEET_HUMANOID0, 0, 4);
inline constexpr int TILE_ORC           = make_tile(SHEET_HUMANOID0, 3, 4);
inline constexpr int TILE_TROLL         = make_tile(SHEET_HUMANOID0, 0, 14);
inline constexpr int TILE_DRAGON        = make_tile(SHEET_REPTILE0, 0, 4);
inline constexpr int TILE_ARCHER        = make_tile(SHEET_HUMANOID0, 2, 12);
inline constexpr int TILE_MAGE          = make_tile(SHEET_HUMANOID0, 5, 2);
inline constexpr int TILE_WOLF          = make_tile(SHEET_DOG0, 0, 1);
inline constexpr int TILE_FIRE_WOLF     = make_tile(SHEET_DOG0, 0, 5);
inline constexpr int TILE_ICE_WOLF      = make_tile(SHEET_DOG0, 2, 1);
inline constexpr int TILE_BAT           = make_tile(SHEET_AVIAN0, 0, 12);
inline constexpr int TILE_KOBOLD        = make_tile(SHEET_PLAYER0, 0, 14);
inline constexpr int TILE_MIMIC         = make_tile(SHEET_MISC0, 0, 0);
inline constexpr int TILE_SHOPKEEPER    = make_tile(SHEET_HUMANOID0, 0, 16);
inline constexpr int TILE_SPIDER_SMALL  = make_tile(SHEET_PEST0, 0, 4);
inline constexpr int TILE_SPIDER_GIANT  = make_tile(SHEET_PEST0, 2, 4);
inline constexpr int TILE_SPIDER_WEAVER = make_tile(SHEET_PEST0, 1, 4);

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------
inline constexpr int TILE_POTION        = make_tile(SHEET_POTION, 0, 0);
inline constexpr int TILE_SCROLL        = make_tile(SHEET_SCROLL, 0, 0);
inline constexpr int TILE_MELEE_WEAPON  = make_tile(SHEET_SHORT_WEP, 0, 0);
inline constexpr int TILE_TWO_HANDED    = make_tile(SHEET_LONG_WEP, 0, 0);
inline constexpr int TILE_RANGED_WEAPON = make_tile(SHEET_SHORT_WEP, 4, 0);
inline constexpr int TILE_ARMOR         = make_tile(SHEET_ARMOR, 0, 0);
inline constexpr int TILE_SHIELD_ITEM   = make_tile(SHEET_SHIELD, 0, 0);
inline constexpr int TILE_HELM          = make_tile(SHEET_HAT, 0, 0);
inline constexpr int TILE_RING          = make_tile(SHEET_RING, 0, 0);
inline constexpr int TILE_AMULET       = make_tile(SHEET_AMULET_ITEM, 0, 0);
inline constexpr int TILE_FOOD          = make_tile(SHEET_FOOD, 0, 0);
inline constexpr int TILE_CORPSE        = make_tile(SHEET_FLESH, 0, 0);
inline constexpr int TILE_GOLD          = make_tile(SHEET_MONEY, 0, 0);
inline constexpr int TILE_GIRDLE        = make_tile(SHEET_ARMOR, 0, 2);
inline constexpr int TILE_GAUNTLETS     = make_tile(SHEET_ARMOR, 0, 3);
inline constexpr int TILE_WEB           = make_tile(SHEET_EFFECT0, 0, 0);
inline constexpr int TILE_AMULET_YENDOR = make_tile(SHEET_AMULET_ITEM, 2, 0);
inline constexpr int TILE_INVISIBLE     = make_tile(SHEET_EFFECT0, 1, 0);

// ---------------------------------------------------------------------------
// Decorations (Decor0/Decor1 -- animated pair for torches/flames)
// ---------------------------------------------------------------------------
inline constexpr int TILE_TORCH_1      = make_tile(SHEET_DECOR0, 0, 7);
inline constexpr int TILE_TORCH_2      = make_tile(SHEET_DECOR0, 1, 7);
inline constexpr int TILE_CAMPFIRE     = make_tile(SHEET_DECOR0, 2, 7);
inline constexpr int TILE_CANDELABRA   = make_tile(SHEET_DECOR0, 4, 7);
inline constexpr int TILE_COBWEB_1     = make_tile(SHEET_DECOR0, 0, 8);
inline constexpr int TILE_COBWEB_2     = make_tile(SHEET_DECOR0, 1, 8);
inline constexpr int TILE_COBWEB_3     = make_tile(SHEET_DECOR0, 2, 8);
inline constexpr int TILE_COBWEB_4     = make_tile(SHEET_DECOR0, 3, 8);
inline constexpr int TILE_VASE_BROWN   = make_tile(SHEET_DECOR0, 0, 3);
inline constexpr int TILE_VASE_BLUE    = make_tile(SHEET_DECOR0, 2, 3);
inline constexpr int TILE_VASE_FANCY   = make_tile(SHEET_DECOR0, 4, 3);
inline constexpr int TILE_POT_SMALL    = make_tile(SHEET_DECOR0, 6, 3);
inline constexpr int TILE_VINE         = make_tile(SHEET_DECOR0, 4, 2);
inline constexpr int TILE_CANDLE_SMALL = make_tile(SHEET_DECOR0, 0, 2);

// ---------------------------------------------------------------------------
// Extra wall autotile groups (6x3 blocks in Wall.png, after 3-row guide)
// ---------------------------------------------------------------------------
inline constexpr WallAutotileGroup WALL_AUTOTILE_BRICK = { SHEET_WALL, 0, 6 };
inline constexpr WallAutotileGroup WALL_AUTOTILE_CAVE  = { SHEET_WALL, 0, 9 };
inline constexpr WallAutotileGroup WALL_AUTOTILE_DARK  = { SHEET_WALL, 0, 12 };

// ---------------------------------------------------------------------------
// Extra floor autotile groups (3x3 blocks in Floor.png)
// ---------------------------------------------------------------------------
inline constexpr AutotileGroup AUTOTILE_FLOOR_DARK = { SHEET_FLOOR, 0, 9 };
inline constexpr AutotileGroup AUTOTILE_FLOOR_TILE = { SHEET_FLOOR, 0, 12 };

// ---------------------------------------------------------------------------
// GUI panel frame tiles (GUI0/GUI1 animated pair)
// Sheet layout (16x16 sprites, 16 cols x 15 rows):
//   Rows 0-3:  Icons (c0-c4) + horizontal bar components (c6-c10)
//   Row  4-5:  Status icons and directional arrows
//   Rows 7-9:  Panel frame sets, each as mini(1-tile) + full 3x3 block:
//     Style A (pink border):   mini=c0, full=c1-c3
//     Style B (red fill):      mini=c4, full=c5-c7
//     Style C (silver border): mini=c8, full=c9-c11   <-- used here
//     Style D (cream fill):    mini=c12, full=c13-c15
//
// Frame set C -- silver/white border, black fill -- rows 7-9, cols 9-11:
inline constexpr int GUI_FRAME_TL = make_tile(SHEET_GUI0,  9, 7);
inline constexpr int GUI_FRAME_T  = make_tile(SHEET_GUI0, 10, 7);
inline constexpr int GUI_FRAME_TR = make_tile(SHEET_GUI0, 11, 7);
inline constexpr int GUI_FRAME_L  = make_tile(SHEET_GUI0,  9, 8);
inline constexpr int GUI_FRAME_F  = make_tile(SHEET_GUI0, 10, 8);
inline constexpr int GUI_FRAME_R  = make_tile(SHEET_GUI0, 11, 8);
inline constexpr int GUI_FRAME_BL = make_tile(SHEET_GUI0,  9, 9);
inline constexpr int GUI_FRAME_B  = make_tile(SHEET_GUI0, 10, 9);
inline constexpr int GUI_FRAME_BR = make_tile(SHEET_GUI0, 11, 9);

// Heart icons -- row 1: red hearts (full → empty), row 2: green, row 3: blue
inline constexpr int GUI_HEART_FULL  = make_tile(SHEET_GUI0, 0, 1);
inline constexpr int GUI_HEART_HALF  = make_tile(SHEET_GUI0, 2, 1);
inline constexpr int GUI_HEART_EMPTY = make_tile(SHEET_GUI0, 3, 1);

// end of file: TileId.h