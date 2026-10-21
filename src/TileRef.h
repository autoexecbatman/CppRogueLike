#pragma once
// file: TileRef.h
//
// A reference to one DawnLike sprite: which sheet, and the column and row in it. Plain data,
// with no raylib and no renderer, so anything that only names a tile - a palette entry, a
// tile in the config, a decoration - can hold one without drawing anything.
//
// Usage:
//   TileRef barrel{ TileSheet::SHEET_DECOR0, 4, 6 };   // sheet, column, row
//   barrel.is_valid();                                // -> true
//   TileRef{}.is_valid();                             // -> false, the "no tile" value

// DawnLike sprite sheet indices.
// Sheets with 0/1 suffixes are animation frame pairs.
enum class TileSheet
{
	SHEET_FLOOR,
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
	// Items -- appended to preserve existing enum integer values
	SHEET_AMMO,
	SHEET_WAND,
	SHEET_BOOK,
	SHEET_BOOT,
	SHEET_GLOVE,
	SHEET_KEY,
	SHEET_LIGHT,
	SHEET_TOOL,
	SHEET_ROCK,
	SHEET_MUSIC,
	SHEET_CHEST0,
	// Characters -- appended to preserve existing enum integer values
	SHEET_SLIME0,
	SHEET_CAT0,
	SHEET_RODENT0,
	SHEET_PLANT0,
	SHEET_ELEMENTAL0,
	SHEET_AQUATIC0,
	// Objects -- appended to preserve existing enum integer values
	SHEET_ORE0,
	SHEET_HILL0,
	SHEET_TREE0,
	SHEET_GROUND0,
	SHEET_TRAP0,
	SHEET_FENCE,
	SHEET_MAP0,
	// Generated item art -- appended to preserve existing enum integer values.
	// One 64px sheet holding every item in items.json, addressed by tiles.json.
	SHEET_ITEMS,
	// Equipment slot symbols, one per slot the inventory draws, so an empty slot can
	// show what belongs in it rather than a dash. Appended for the same reason as the
	// line above: inserting anywhere else renumbers every sheet after it.
	//
	// Declared ahead of its art. The generator reads this enum to learn the sheet
	// number and refuses to install until the entry exists, and a loader pointing at a
	// PNG that is not there yet is the broken half of that chicken and egg - so the
	// load_static call lands in the same pass as the image.
	SHEET_SLOTS,
	// The menu frame's nine cells. Its own static sheet rather than cells inside GUI0,
	// which is loaded animated: a border that flickers is a border nobody wants.
	// Declared ahead of its art for the same reason as SHEET_SLOTS - the generator reads
	// this enum for the sheet number and refuses to install until the entry exists.
	SHEET_FRAME,
	// The health and hunger bars: a rim, a fill the code tints, and the two icons
	// beside them. A 32px cell, because a bar is a text row tall at every zoom, so
	// the art is never scaled vertically. Declared ahead of its art for the same
	// reason as SHEET_SLOTS.
	SHEET_BARS,
	COUNT, // sentinel -- keep last
};

// Unencoded tile reference: sheet + grid position.
// Default-constructed TileRef (col == -1) means "no tile".
struct TileRef
{
	TileSheet sheet{};
	int col{ -1 };
	int row{ -1 };

	[[nodiscard]] bool is_valid() const noexcept { return col >= 0; }

	[[nodiscard]] bool operator==(const TileRef& other) const noexcept
	{
		return sheet == other.sheet && col == other.col && row == other.row;
	}
};
