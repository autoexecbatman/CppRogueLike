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
class PrefabLibrary;
class RandomDice;
struct GameContext;

class DungeonGenerator
{
public:
	void generate(
		int mapWidth,
		int mapHeight,
		RandomDice& rng,
		bool withActors,
		GameContext& ctx,
		Map& map) const;

private:
	std::vector<DungeonRoom> place_rooms(
		int mapWidth,
		int mapHeight,
		RandomDice& rng,
		const PrefabLibrary* prefabs) const;

	// Phase 1: Prim's MST — guarantees full connectivity, no cycles.
	// Room types must be assigned after this and before add_extra_corridors.
	void connect_spanning_tree(
		std::vector<DungeonRoom>& rooms) const;

	// Phase 2: extra short-range corridors to create loop paths.
	// Does not affect leaf detection — run after room type assignment.
	void add_extra_corridors(
		std::vector<DungeonRoom>& rooms,
		RandomDice& rng) const;

	void connect_pair(
		std::vector<DungeonRoom>& rooms,
		int indexA,
		int indexB) const;
};
