// file: ContentRegistry.cpp
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ContentRegistry.h"
#include "TileRef.h"

namespace CppRL
{

TileRef ContentRegistry::get_tile(std::string_view key) const
{
	return itemTiles.contains(std::string{ key }) ?
		itemTiles.at(std::string{ key }) :
		TileRef{};
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
			std::format(
				"ContentRegistry::remove_tile -- no tile for '{}'",
				key));
	}
}

const std::unordered_map<std::string, TileRef>& ContentRegistry::all_tiles()
	const
{
	return itemTiles;
}

// end of file: ContentRegistry.cpp

} // namespace CppRL
