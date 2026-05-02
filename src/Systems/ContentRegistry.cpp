// file: ContentRegistry.cpp
#include <string>

#include "../Renderer/Renderer.h"
#include "ContentRegistry.h"

TileRef ContentRegistry::get_tile(std::string_view key) const
{
	return itemTiles.contains(std::string{ key }) ? itemTiles.at(std::string{ key }) : TileRef{};
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
