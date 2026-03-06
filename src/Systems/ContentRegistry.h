// file: ContentRegistry.h
#pragma once

#include <string>
#include <unordered_map>

#include "../Items/ItemClassification.h"
#include "../Renderer/Renderer.h"

// ContentRegistry -- singleton source of truth for item-to-tile mappings.
//
// JSON from data/content/tiles.json is the sole source of item tile assignments.
// ContentEditor calls set_tile() + save() to author the JSON.
// ItemCreator reads via get_tile() instead of hardcoded symbols.
//
// Monster tile assignments have moved to MonsterCreator / data/content/monsters.json.

class ContentRegistry
{
public:
	[[nodiscard]] static ContentRegistry& instance();

	[[nodiscard]] TileRef get_tile(ItemId id) const;
	void set_tile(ItemId id, TileRef tile);

	[[nodiscard]] static std::string_view item_key(ItemId id);

private:
	ContentRegistry() = default;

	std::unordered_map<int, TileRef> m_item_tiles;
};

// end of file: ContentRegistry.h
