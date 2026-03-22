// file: ContentRegistry.cpp
#include <string>

#include "../Renderer/Renderer.h"
#include "ContentRegistry.h"

TileRef ContentRegistry::get_tile(std::string_view key) const
{
	auto it = m_item_tiles.find(std::string{ key });
	return it != m_item_tiles.end() ? it->second : TileRef{};
}

void ContentRegistry::set_tile(std::string_view key, TileRef tile)
{
	m_item_tiles[std::string{ key }] = tile;
}

const std::unordered_map<std::string, TileRef>& ContentRegistry::all_tiles() const
{
	return m_item_tiles;
}

// end of file: ContentRegistry.cpp
