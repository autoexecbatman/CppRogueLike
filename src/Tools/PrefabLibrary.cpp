// file: PrefabLibrary.cpp
#include <fstream>
#include <cmath>
#include <algorithm>
#include <format>

#include <nlohmann/json.hpp>

#include "PrefabLibrary.h"
#include "DecorEditor.h"
#include "../Renderer/TileId.h"
#include "../Map/DungeonRoom.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Symbol map
// ---------------------------------------------------------------------------

void PrefabLibrary::build_structural_symbols()
{
    symbol_to_tile.clear();
    symbol_to_label.clear();
    palette_order.clear();

    auto reg = [&](char sym, std::string label)
    {
        symbol_to_tile[sym]  = 0;
        symbol_to_label[sym] = label;
        palette_order.emplace_back(sym, std::move(label));
    };

    // Structural markers -- no decoration sprite (tile_id = 0)
    reg('#', "Wall");
    reg('.', "Floor");
    reg(',', "Corridor");
    reg('+', "Door");
    reg('~', "Water");
}

void PrefabLibrary::load_tile_labels(std::string_view path)
{
    build_structural_symbols();

    std::ifstream in(path.data());
    if (!in.is_open())
        return;

    json j;
    try
    {
        in >> j;
        for (const auto& e : j["tiles"])
        {
            std::string sym_str = e.value("symbol", "");
            if (sym_str.empty())
                continue;

            char sym      = sym_str[0];
            int  sheet    = e.value("sheet", 0);
            int  col      = e.value("col",   0);
            int  row      = e.value("row",   0);
            int  tile_id  = make_tile(sheet, col, row);
            std::string label = e.value("label", sym_str);

            symbol_to_tile[sym]  = tile_id;
            symbol_to_label[sym] = label;
            palette_order.emplace_back(sym, label);
        }
    }
    catch (...) {}
}

void PrefabLibrary::load_symbol_map(const json& j)
{
    if (!j.contains("symbol_map"))
        return;

    for (const auto& [key, val] : j["symbol_map"].items())
    {
        if (key.empty())
            continue;
        char    sym   = key[0];
        int     tile  = val.value("tile_id", symbol_to_tile.count(sym) ? symbol_to_tile[sym] : 0);
        std::string lbl = val.value("label",  symbol_to_label.count(sym) ? symbol_to_label[sym] : key);

        symbol_to_tile[sym]  = tile;
        symbol_to_label[sym] = lbl;

        // Keep palette_order consistent -- update existing or append
        bool found = false;
        for (auto& entry : palette_order)
        {
            if (entry.first == sym)
            {
                entry.second = lbl;
                found = true;
                break;
            }
        }
        if (!found)
            palette_order.emplace_back(sym, lbl);
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

int PrefabLibrary::resolve_decor(char c) const
{
    auto it = symbol_to_tile.find(c);
    return (it != symbol_to_tile.end()) ? it->second : 0;
}

bool PrefabLibrary::is_decoration(char c) const
{
    return resolve_decor(c) != 0;
}

std::string PrefabLibrary::symbol_label(char c) const
{
    auto it = symbol_to_label.find(c);
    return (it != symbol_to_label.end()) ? it->second : std::string(1, c);
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PrefabLibrary::load(std::string_view path)
{
    std::ifstream in(path.data());
    if (!in.is_open())
        return;

    json j;
    try
    {
        in >> j;

        if (!j.contains("prefabs"))
            return;

        prefabs.clear();
        for (const auto& entry : j["prefabs"])
        {
            Prefab p;
            p.name      = entry.value("name",      "unnamed");
            p.depth_min = entry.value("depth_min", 1);
            p.depth_max = entry.value("depth_max", 99);
            p.weight    = entry.value("weight",    1.0f);

            if (entry.contains("rows"))
            {
                for (const auto& row : entry["rows"])
                    p.rows.push_back(row.get<std::string>());
            }

            if (!p.rows.empty())
                prefabs.push_back(std::move(p));
        }
    }
    catch (...) {}  // Corrupt file -- keep defaults
}

void PrefabLibrary::save(std::string_view path) const
{
    json j;
    json arr = json::array();
    for (const auto& p : prefabs)
    {
        json entry;
        entry["name"]      = p.name;
        entry["depth_min"] = p.depth_min;
        entry["depth_max"] = p.depth_max;
        entry["weight"]    = p.weight;
        entry["rows"]      = p.rows;
        arr.push_back(entry);
    }
    j["prefabs"] = arr;

    std::ofstream out(path.data());
    out << j.dump(2);
}

void PrefabLibrary::set_symbol_tile(char sym, int tile_id)
{
    symbol_to_tile[sym] = tile_id;
}

void PrefabLibrary::set_symbol_label(char sym, const std::string& label)
{
    symbol_to_label[sym] = label;
    for (auto& entry : palette_order)
    {
        if (entry.first == sym)
        {
            entry.second = label;
            break;
        }
    }
}

void PrefabLibrary::add_or_replace(Prefab p)
{
    for (auto& existing : prefabs)
    {
        if (existing.name == p.name)
        {
            existing = std::move(p);
            return;
        }
    }
    prefabs.push_back(std::move(p));
}

void PrefabLibrary::remove(const std::string& name)
{
    std::erase_if(prefabs, [&](const Prefab& p) { return p.name == name; });
}

// ---------------------------------------------------------------------------
// Prefab selection
// ---------------------------------------------------------------------------

int PrefabLibrary::pick_prefab(
    int  total_w,
    int  total_h,
    int  dungeon_level,
    long hash_val
) const
{
    struct Candidate
    {
        int   index;
        float weight;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(prefabs.size());

    for (int i = 0; i < static_cast<int>(prefabs.size()); ++i)
    {
        const Prefab& p = prefabs[i];
        if (p.depth_min > dungeon_level || p.depth_max < dungeon_level)
            continue;
        if (p.width()  > total_w) continue;
        if (p.height() > total_h) continue;
        candidates.push_back({ i, p.weight });
    }

    if (candidates.empty())
        return -1;

    float total_weight = 0.0f;
    for (const auto& c : candidates)
        total_weight += c.weight;

    // Map hash to [0, total_weight) deterministically
    float selection = static_cast<float>(std::abs(hash_val) % 1000000) / 1000000.0f * total_weight;

    float accumulated = 0.0f;
    for (const auto& c : candidates)
    {
        accumulated += c.weight;
        if (selection < accumulated)
            return c.index;
    }
    return candidates.back().index;
}

// ---------------------------------------------------------------------------
// Apply to rooms
// ---------------------------------------------------------------------------

void PrefabLibrary::apply_to_rooms(
    const std::vector<DungeonRoom>& rooms,
    long                            seed,
    int                             dungeon_level,
    DecorEditor&                    editor
) const
{
    if (prefabs.empty() || rooms.empty())
        return;

    auto room_hash = [&](int room_idx) -> long
    {
        long h = seed ^ (static_cast<long>(room_idx) * 2654435761L);
        h ^= h >> 16;
        h *= 0x45d9f3bL;
        h ^= h >> 16;
        return h;
    };

    for (int room_idx = 0; room_idx < static_cast<int>(rooms.size()); ++room_idx)
    {
        const DungeonRoom& room = rooms[static_cast<size_t>(room_idx)];

        // Total room dimensions including surrounding walls
        const int total_w = room.total_width();
        const int total_h = room.total_height();

        int idx = pick_prefab(total_w, total_h, dungeon_level, room_hash(room_idx));
        if (idx < 0)
            continue;

        const Prefab& p = prefabs[idx];

        // Center the prefab inside the room.
        // Prefab origin [0,0] aligns to the left/top wall so the '#' border
        // of an exactly-fitting prefab lines up with the room walls.
        const int base_x   = room.left_wall();
        const int base_y   = room.top_wall();
        const int offset_x = (total_w - p.width())  / 2;
        const int offset_y = (total_h - p.height()) / 2;

        // Stamp the decoration layer -- floor tiles only.
        for (int r = 0; r < p.height(); ++r)
        {
            const std::string& row_str = p.rows[r];
            for (int c = 0; c < static_cast<int>(row_str.size()); ++c)
            {
                char sym     = row_str[c];
                int  tile_id = resolve_decor(sym);
                if (tile_id == 0)
                    continue;

                const int world_x = base_x + offset_x + c;
                const int world_y = base_y + offset_y + r;

                // Guard: only place on the actual floor interior.
                if (world_x < room.col || world_x > room.col_end()) continue;
                if (world_y < room.row || world_y > room.row_end()) continue;

                editor.place_tile(world_x, world_y, tile_id);
            }
        }
    }
}
