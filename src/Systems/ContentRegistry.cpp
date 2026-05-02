// file: ContentRegistry.cpp
#include <string>

#include "../Renderer/Renderer.h"
#include "ContentRegistry.h"

TileRef ContentRegistry::get_tile(std::string_view key) const
{
	// TODO: replace iterator pattern with contains + at
	auto it = itemTiles.find(std::string{ key });
	return it != itemTiles.end() ? it->second : TileRef{};
}

void ContentRegistry::set_tile(std::string_view key, TileRef tile)
{
	itemTiles[std::string{ key }] = tile;
}

const std::unordered_map<std::string, TileRef>& ContentRegistry::all_tiles() const
{
	return itemTiles;
}

// end of file: ContentRegistry.cpp
