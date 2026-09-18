#pragma once

// file: Map/TreasureRoom.h
//
// A locked room with treasure in it and a jailer outside holding the key.
// What goes into a room is a different question from what shape the room is,
// so this sits beside the map rather than inside it, the way the encounter
// planner already does.
//
// Usage, from dungeon generation once the rooms and the stairs exist:
//
//   #include "TreasureRoom.h"
//
//   // dungeonLevel raises both the chance and the quality of the hoard.
//   // generationRng is the map's own seeded stream, used for the warden's name.
//   TreasureRoom::maybe_create(3, mapRng, ctx);   // -> true if a room was taken
//
// The map, the dice, the rooms, the stairs and the registries are all reached
// through ctx, and a context missing any of them is a wiring fault rather than
// a runtime case: every entry point asserts rather than returning quietly.

struct GameContext;
struct DungeonRoom;
class Map;
class RandomDice;

namespace TreasureRoom
{
	// Rolls for a treasure room on this level and builds one if it lands, and
	// answers whether it did. Refuses a missed roll, a level of fewer than two
	// rooms, and any level whose every room has more than one door. Never takes
	// the room holding the stairs: locking that one strands the player.
	//
	// Example:
	//   TreasureRoom::maybe_create(1, mapRng, ctx);   // -> false, roll missed
	bool maybe_create(int dungeonLevel, RandomDice& generationRng, GameContext& ctx);

	// Turns one room into a treasure room: floors it, puts a hoard of the given
	// quality at its centre, spawns the named warden and its escort, then calls
	// setup_guard to seal it. Quality runs 1 to 3 and buys more escorts.
	//
	// Example:
	//   TreasureRoom::create(rooms.at(2), 3, mapRng, ctx);   // richest hoard
	void create(
		const DungeonRoom& room,
		int quality,
		RandomDice& generationRng,
		GameContext& ctx);

	// Locks every door into the room and spawns the jailer carrying the key on
	// the corridor side of one. Unlocks everything again if no reachable spawn
	// exists, since a room nobody can open is worse than an unguarded one.
	//
	// Example:
	//   TreasureRoom::setup_guard(room, ctx);   // room sealed, jailer outside
	void setup_guard(const DungeonRoom& room, GameContext& ctx);

	// How many doors on the room's wall border open into it. A treasure room
	// needs exactly one, so locking it cannot cut the dungeon in two.
	//
	// Example:
	//   TreasureRoom::count_entrances(*ctx.map, room);   // -> 1
	int count_entrances(const Map& map, const DungeonRoom& room);
}
