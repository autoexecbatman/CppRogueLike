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
    SHEET_COUNT
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
inline constexpr int TILE_WATER       = make_tile(SHEET_FLOOR, 1, 13);  // dark navy floor
inline constexpr int TILE_WALL_STONE  = make_tile(SHEET_WALL, 1, 5);    // stone wall body
inline constexpr int TILE_DOOR_CLOSED = make_tile(SHEET_DOOR0, 0, 0);   // solid closed door
inline constexpr int TILE_DOOR_OPEN   = make_tile(SHEET_DOOR0, 0, 3);   // open door frame
inline constexpr int TILE_STAIRS      = make_tile(SHEET_TILE, 0, 2);    // staircase

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
inline constexpr AutotileGroup AUTOTILE_WALL_STONE  = { SHEET_WALL, 0, 4 };

// Compute autotile variant from 4-neighbor presence flags.
// n/e/s/w = true when same-type tile exists in that direction.
constexpr int autotile_resolve(AutotileGroup group, bool n, bool e, bool s, bool w)
{
    int col_offset = (!w && e) ? 0 : (w && !e) ? 2 : 1;
    int row_offset = (!n && s) ? 0 : (n && !s) ? 2 : 1;
    return make_tile(group.sheet, group.origin_col + col_offset, group.origin_row + row_offset);
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------
inline constexpr int TILE_PLAYER = make_tile(SHEET_PLAYER0, 0, 0);

// ---------------------------------------------------------------------------
// Monsters
// ---------------------------------------------------------------------------
inline constexpr int TILE_GOBLIN        = make_tile(SHEET_HUMANOID0, 0, 11);
inline constexpr int TILE_ORC           = make_tile(SHEET_HUMANOID0, 0, 0);
inline constexpr int TILE_TROLL         = make_tile(SHEET_HUMANOID0, 0, 20);
inline constexpr int TILE_DRAGON        = make_tile(SHEET_REPTILE0, 0, 4);
inline constexpr int TILE_ARCHER        = make_tile(SHEET_HUMANOID0, 2, 3);
inline constexpr int TILE_MAGE          = make_tile(SHEET_HUMANOID0, 0, 9);
inline constexpr int TILE_WOLF          = make_tile(SHEET_DOG0, 0, 0);
inline constexpr int TILE_FIRE_WOLF     = make_tile(SHEET_DOG0, 5, 0);
inline constexpr int TILE_ICE_WOLF      = make_tile(SHEET_DOG0, 2, 1);
inline constexpr int TILE_BAT           = make_tile(SHEET_AVIAN0, 0, 12);
inline constexpr int TILE_KOBOLD        = make_tile(SHEET_REPTILE0, 0, 0);
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

// end of file: TileId.h
