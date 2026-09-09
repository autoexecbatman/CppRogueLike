#pragma once

#include <string>

#include "../Actor/Actor.h"

// file: TileDefinition.h
//
// Everything a tile type does, in one place, loaded from
// data/tiles/tile_config.json.
//
// Tile behaviour used to be six switch statements in five files - one deciding
// whether a tile blocks, one its minimap colour, one its tooltip, one what the
// player hears standing on it, one what they hear when blocked by it, and one
// whether pathfinding may cross it. Five of the six carried a default case, so
// adding a tile type compiled clean and the new tile silently behaved like
// whatever the default said.
//
// Nothing here is code. Every one of those switches produced data, including
// the swim rule, which is "blocks, unless the creature has this state".
//
// Usage -- asking whether a creature may enter a tile:
//
//   const TileDefinition& definition = ctx.tileConfig->get_tile_definition(tileType);
//   if (!definition.blocksMovement) { /* open ground */ }
//   if (definition.bypassState && creature.has_state(*definition.bypassState))
//   {
//       // a spider crossing water
//   }

// A colour as authored, kept free of the renderer so map and minimap code can
// take a definition without pulling raylib in.
struct TileColor
{
	int red{ 0 };
	int green{ 0 };
	int blue{ 0 };
	int alpha{ 255 };
};

struct TileDefinition
{
	std::string displayName{ "Unknown" }; // shown in the hover tooltip

	bool blocksMovement{ true }; // whether a creature is stopped by it

	// An ability that lets a creature through anyway. CAN_SWIM for water; unset
	// for a wall, which nothing crosses.
	bool hasBypassState{ false };
	ActorState bypassState{ ActorState::CAN_SWIM };

	// Said once on entering, and once when the tile turns a move away. Empty
	// means the tile is unremarkable and says nothing.
	std::string entryMessage{};
	std::string blockedMessage{};

	TileColor minimapVisible{}; // drawn inside the field of view
	TileColor minimapRemembered{}; // drawn from memory, outside it
};
