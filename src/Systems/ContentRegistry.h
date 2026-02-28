// file: ContentRegistry.h
#pragma once

#include <string>
#include <unordered_map>

#include "../Factories/MonsterCreator.h"
#include "../Items/ItemClassification.h"
#include "../Renderer/Renderer.h"

// ContentRegistry -- singleton source of truth for all entity-to-tile mappings.
//
// JSON from data/content/tiles.json is the sole source of tile assignments.
// GameEditor calls set_tile() + save() to author the JSON.
// ItemCreator and MonsterCreator read via get_tile() instead of hardcoded symbols.

class ContentRegistry
{
public:
	[[nodiscard]] static ContentRegistry& instance();

	[[nodiscard]] TileRef get_tile(ItemId id) const;
	[[nodiscard]] TileRef get_tile(MonsterId id) const;
	void set_tile(ItemId id, TileRef tile);
	void set_tile(MonsterId id, TileRef tile);

	[[nodiscard]] static std::string_view item_key(ItemId id);
	[[nodiscard]] static std::string_view monster_key(MonsterId id);

private:
	ContentRegistry() = default;

	std::unordered_map<int, TileRef> m_item_tiles;
	std::unordered_map<int, TileRef> m_monster_tiles;
};

// end of file: ContentRegistry.h
