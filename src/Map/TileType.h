#pragma once

// file: TileType.h
//
// The kind of ground a map cell is. Kept apart from Map so a definition table,
// a minimap or a tooltip can take a tile type without the whole map.
//
// What each type does - whether it blocks, what crosses it anyway, what it says
// and how it draws - is data, in TileDefinition, authored in
// data/tiles/tile_config.json. Adding a value here means adding an entry there.
//
// Usage:
//
//   if (map.get_tile_type(position) == TileType::WATER)  // the kind of cell
//   {
//       // ask TileConfig what water does, rather than deciding here
//   }

enum class TileType
{
	FLOOR,
	WALL,
	WATER,
	CLOSED_DOOR,
	OPEN_DOOR,
	CORRIDOR,
	// Add more as needed...
};
