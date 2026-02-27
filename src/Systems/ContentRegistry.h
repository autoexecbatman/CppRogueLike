// file: ContentRegistry.h
#pragma once

#include <string_view>
#include <unordered_map>

#include "../Items/ItemClassification.h"
#include "../Factories/MonsterCreator.h"
#include "../Renderer/TileId.h"

// ContentRegistry -- singleton source of truth for all entity-to-tile mappings.
//
// Bootstrap defaults are established from TileId.h constants in the constructor.
// JSON overrides from data/content/tiles.json are layered on top via load().
// GameEditor calls set_tile() + save() to author the JSON.
// ItemCreator and MonsterCreator read via get_tile() instead of hardcoded symbols.

class ContentRegistry
{
public:
    [[nodiscard]] static ContentRegistry& instance();

    void load(std::string_view path);
    void save(std::string_view path) const;

    [[nodiscard]] int get_tile(ItemId id) const;
    [[nodiscard]] int get_tile(MonsterId id) const;
    void set_tile(ItemId id, int tile_id);
    void set_tile(MonsterId id, int tile_id);

    [[nodiscard]] static std::string_view item_key(ItemId id);
    [[nodiscard]] static std::string_view monster_key(MonsterId id);

private:
    ContentRegistry();
    void bootstrap_items();
    void bootstrap_monsters();

    std::unordered_map<int, int> m_item_tiles;
    std::unordered_map<int, int> m_monster_tiles;
};

// end of file: ContentRegistry.h
