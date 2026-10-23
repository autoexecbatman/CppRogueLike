// file: ContentRegistry.h
#pragma once

#include <string>
#include <unordered_map>

#include "TileRef.h"

// ContentRegistry -- value-type source of truth for item-to-tile mappings.
//
// Owned by Game, threaded via GameContext.
// JSON from data/content/tiles.json is the sole source of item tile assignments.
// ContentEditor calls set_tile() to author, ContentRegistryIO to persist.
// ItemCreator::create() reads it through ctx to resolve tiles at spawn time.

class ContentRegistry
{
private:
	std::unordered_map<std::string, TileRef> itemTiles;

public:
	ContentRegistry() = default;

	[[nodiscard]] TileRef get_tile(std::string_view key) const;
	void set_tile(std::string_view key, TileRef tile);

	// Forgets an item's sprite, for when the item itself is removed. Throws
	// std::out_of_range for a key holding no sprite.
	void remove_tile(std::string_view key);
	[[nodiscard]] const std::unordered_map<std::string, TileRef>& all_tiles()
		const;
};

// end of file: ContentRegistry.h
