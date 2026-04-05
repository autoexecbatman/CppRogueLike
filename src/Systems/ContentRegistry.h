// file: ContentRegistry.h
#pragma once

#include <string>
#include <unordered_map>

#include "../Renderer/Renderer.h"

// ContentRegistry -- value-type source of truth for item-to-tile mappings.
//
// Owned by Game, threaded via GameContext.
// JSON from data/content/tiles.json is the sole source of item tile assignments.
// ContentEditor calls set_tile() to author, ContentRegistryIO to persist.
// ItemCreator::create() receives a ContentRegistry& to resolve tiles at spawn time.

class ContentRegistry
{
public:
	ContentRegistry() = default;

	[[nodiscard]] TileRef get_tile(std::string_view key) const;
	void set_tile(std::string_view key, TileRef tile);
	[[nodiscard]] const std::unordered_map<std::string, TileRef>& all_tiles() const;

private:
	std::unordered_map<std::string, TileRef> itemTiles;
};

// end of file: ContentRegistry.h
