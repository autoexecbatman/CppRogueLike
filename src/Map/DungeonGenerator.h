#pragma once
// file: DungeonGenerator.h
//
// Grid-cell room placement + Prim's MST corridors.
//
// Algorithm:
//   1. Divide the map into fixed-size cells.
//      Each cell holds at most one room, so rooms never overlap.
//   2. For each cell, optionally place a room of random size within it.
//   3. Sort rooms top-left first so room[0] is always the player start.
//   4. Build a minimum spanning tree (Prim) connecting every room.
//   5. Add ~20 % extra short-range edges to create loop paths.

#include <vector>
#include "DungeonRoom.h"

class Map;
class TCODRandom;
struct GameContext;

class DungeonGenerator
{
public:
    void generate(
        int          map_width,
        int          map_height,
        TCODRandom&  rng,
        bool         withActors,
        GameContext& ctx,
        Map&         map
    ) const;

private:
    std::vector<DungeonRoom> place_rooms(
        int         map_width,
        int         map_height,
        TCODRandom& rng
    ) const;

    void connect_all(
        const std::vector<DungeonRoom>& rooms,
        TCODRandom&                     rng,
        Map&                            map
    ) const;

    void connect_pair(
        const DungeonRoom& a,
        const DungeonRoom& b,
        Map&               map
    ) const;
};
