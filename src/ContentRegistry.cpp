// file: ContentRegistry.cpp
#include <format>
#include <stdexcept>
#include <string>

#include "ContentRegistry.h"
#include "Renderer.h"

TileRef ContentRegistry::get_tile(std::string_view key) const
{
	return itemTiles.contains(std::string{ key }) ? itemTiles.at(std::string{ key }) : TileRef{};
}

void ContentRegistry::set_tile(std::string_view key, TileRef tile)
{
	itemTiles[std::string{ key }] = tile;
}

void ContentRegistry::remove_tile(std::string_view key)
{
	if (itemTiles.erase(std::string{ key }) == 0)
	{
		throw std::out_of_range(
			std::format("ContentRegistry::remove_tile -- no tile for '{}'", key));
	}
}

const std::unordered_map<std::string, TileRef>& ContentRegistry::all_tiles() const
{
	return itemTiles;
}

// end of file: ContentRegistry.cpp
