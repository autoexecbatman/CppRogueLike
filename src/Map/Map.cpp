// file: Map.cpp
#include <iostream>
#include <random>
#include <algorithm>
#include <span>
#include <limits>
#include <cmath>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "Map.h"
#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Destructible.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Gold.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/Monsters.h"
#include "../Factories/MonsterCreator.h"
#include "../ActorTypes/Player.h"
#include "../Ai/Ai.h"
#include "../Ai/AiShopkeeper.h"
#include "../Colors/Colors.h"
#include "../Random/RandomDice.h"
#include "../Items/Weapons.h"
#include "../Items/Items.h"
#include "../Ai/AiMonsterRanged.h"
#include "../Items/Food.h"
#include "../Factories/MonsterFactory.h"
#include "../Factories/ItemFactory.h"
#include "../Factories/ItemCreator.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/LevelManager.h"

// tcod path listener
class PathListener : public ITCODPathCallback
{
private:
	Map& map;

public:
	PathListener(Map& map) noexcept : map(map) {}

	// callback to handle pathfinding
	// returns the cost of the path from (xFrom, yFrom) to (xTo, yTo)
	// if the cost is 0, the path is blocked
	// if the cost is 1, the path is open
	// if the path is empty returns an empty path
	float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const override
	{
		return map.tcodMap->isWalkable(xTo, yTo) ? 1.0f : 0.0f;
	}
};

//====

// a binary space partition listener class (BSP)
class BspListener : public ITCODBspCallback
{
private:
	Map& map; // a map to dig
	GameContext& ctx; // Game context for dependency access
	int roomNum{ 0 }; // room number
	Vector2D lastRoomCenter{ 0,0 }; // center of the last room
	Vector2D lastRoomSize{ 0,0 }; // size of the last room
	Vector2D lastRoomBegin{ 0,0 }; // begin of the last room
	Vector2D lastRoomEnd{ 0,0 }; // end of the last room
	Vector2D lastRoomTopMid{ 0,0 };
	Vector2D lastRoomBottomMid{ 0,0 };
	Vector2D lastRoomRightMid{ 0,0 };
	Vector2D lastRoomLeftMid{ 0,0 };
public:
	BspListener(Map& map, GameContext& ctx) noexcept : map(map), ctx(ctx), roomNum(0) {}

	bool visitNode(TCODBsp* node, void* userData) override
	{
		if (node->isLeaf())
		{
			const bool withActors = static_cast<bool>(userData);

			// Calculate room size
			Vector2D currentRoomSize
			{
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->h - 2), // random int from min size to height - 2
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->w - 2)  // random int from min size to width - 2
			};

			// Calculate room position
			Vector2D currentRoomBegin
			{
				map.rng_unique->getInt(node->y + 1, node->y + node->h - currentRoomSize.y - 1), // from node y + 1 to node height - room height - 1
				map.rng_unique->getInt(node->x + 1, node->x + node->w - currentRoomSize.x - 1)  // from node x + 1 to node width - room width - 1
			};

			// Calculate room end coordinates
			Vector2D currentRoomEnd
			{
				currentRoomBegin.y + currentRoomSize.y,
				currentRoomBegin.x + currentRoomSize.x
			};

			// Create the room
			map.create_room(
				roomNum == 0,
				currentRoomBegin.x,
				currentRoomBegin.y,
				currentRoomBegin.x + currentRoomSize.x - 2,
				currentRoomBegin.y + currentRoomSize.y - 2,
				withActors,
				ctx
			);

			// Calculate room center
			Vector2D currentRoomCenter
			{
				currentRoomBegin.y + (currentRoomSize.y / 2),
				currentRoomBegin.x + (currentRoomSize.x / 2)
			};

			Vector2D currentRoomTopMid
			{
				currentRoomBegin.y,
				currentRoomBegin.x + (currentRoomSize.x / 2)
			};

			Vector2D currentRoomBottomMid
			{
				currentRoomEnd.y - 1,
				currentRoomBegin.x + (currentRoomSize.x / 2)
			};

			Vector2D currentRoomLeftMid
			{
				currentRoomBegin.y + (currentRoomSize.y / 2),
				currentRoomBegin.x
			};

			Vector2D currentRoomRightMid
			{
				currentRoomBegin.y + (currentRoomSize.y / 2),
				currentRoomEnd.x - 1
			};

			int verticalMidPoint = (lastRoomBegin.y + currentRoomEnd.y) / 2;

			int horizontalMidPoint = (lastRoomBegin.x + currentRoomEnd.x) / 2;

			bool isTopGeneralLeft = lastRoomBegin.y > currentRoomEnd.y && lastRoomCenter.x < currentRoomCenter.x;
			bool isTopGeneralRight = lastRoomBegin.y > currentRoomEnd.y && lastRoomCenter.x > currentRoomCenter.x;

			bool isLeftGeneralTop = lastRoomEnd.x > currentRoomBegin.x && lastRoomCenter.y > currentRoomCenter.y;
			bool isLeftGeneralBottom = lastRoomEnd.x > currentRoomBegin.x && lastRoomCenter.y < currentRoomCenter.y;

			bool isBottomGeneralLeft = lastRoomEnd.y < currentRoomBegin.y && lastRoomCenter.x < currentRoomCenter.x;
			bool isBottomGeneralRight = lastRoomEnd.y < currentRoomBegin.y && lastRoomCenter.x > currentRoomCenter.x;

			bool isRightGeneralTop = lastRoomBegin.x < currentRoomEnd.x && lastRoomCenter.y > currentRoomCenter.y;
			bool isRightGeneralBottom = lastRoomBegin.x < currentRoomEnd.x && lastRoomCenter.y < currentRoomCenter.y;

			// Connect this room to the previous room (except for the first room)
			if (roomNum != 0)
			{
				if (isTopGeneralLeft || isTopGeneralRight)
				{
					map.dig_corridor(lastRoomTopMid, Vector2D{ verticalMidPoint, lastRoomCenter.x });
					map.dig_corridor(Vector2D{ verticalMidPoint, lastRoomCenter.x }, Vector2D{ verticalMidPoint, currentRoomCenter.x });
					map.dig_corridor(Vector2D{ verticalMidPoint, currentRoomCenter.x }, currentRoomBottomMid);
				}
				else if (isLeftGeneralTop || isLeftGeneralBottom)
				{
					map.dig_corridor(lastRoomLeftMid, Vector2D{ lastRoomCenter.y, horizontalMidPoint });
					map.dig_corridor(Vector2D{ lastRoomCenter.y, horizontalMidPoint }, Vector2D{ currentRoomCenter.y, horizontalMidPoint });
					map.dig_corridor(Vector2D{ currentRoomCenter.y, horizontalMidPoint }, currentRoomRightMid);
				}
				else if (isBottomGeneralLeft || isBottomGeneralRight)
				{
					map.dig_corridor(lastRoomBottomMid, Vector2D{ verticalMidPoint, lastRoomCenter.x });
					map.dig_corridor(Vector2D{ verticalMidPoint, lastRoomCenter.x }, Vector2D{ verticalMidPoint, currentRoomCenter.x });
					map.dig_corridor(Vector2D{ verticalMidPoint, currentRoomCenter.x }, currentRoomTopMid);
				}
				else if (isRightGeneralTop || isRightGeneralBottom)
				{
					map.dig_corridor(lastRoomRightMid, Vector2D{ lastRoomCenter.y, horizontalMidPoint });
					map.dig_corridor(Vector2D{ lastRoomCenter.y, horizontalMidPoint }, Vector2D{ currentRoomCenter.y, horizontalMidPoint });
					map.dig_corridor(Vector2D{ currentRoomCenter.y, horizontalMidPoint }, currentRoomLeftMid);
				}
				else
				{
					map.dig_corridor(lastRoomCenter, currentRoomCenter);
				}
			}

			// Save current room info for next room connection
			lastRoomCenter = currentRoomCenter;
			lastRoomSize = currentRoomSize;
			lastRoomBegin = currentRoomBegin;
			lastRoomEnd = currentRoomEnd;
			lastRoomTopMid = currentRoomTopMid;
			lastRoomBottomMid = currentRoomBottomMid;
			lastRoomLeftMid = currentRoomLeftMid;
			lastRoomRightMid = currentRoomRightMid;

			roomNum++;
		}
		return true;
	}
};

//====
Map::Map(int map_height, int map_width)
	:
	map_height(map_height),
	map_width(map_width),
	tcodMap(std::make_unique<TCODMap>(map_width, map_height)),
	seed(0), // Will be set in init() with GameContext access
	monsterFactory(std::make_unique<MonsterFactory>()),
	itemFactory(std::make_unique<ItemFactory>())
{
}

bool Map::in_bounds(Vector2D pos) const noexcept
{
	bool result = pos.y >= 0 && pos.y < map_height && pos.x >= 0 && pos.x < map_width;
	// Logging moved to callers that have GameContext access
	return result;
}

void Map::init_tiles(GameContext& ctx)
{
	if (!tiles.empty()) { tiles.clear(); }

	if (ctx.message_system)
	{
		ctx.message_system->log("init_tiles: Creating " + std::to_string(map_width * map_height) + " tiles (" + std::to_string(map_width) + " x " + std::to_string(map_height) + ")");
	}

	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			tiles.emplace_back(Tile(Vector2D{ y, x }, TileType::WALL, 0));
		}
	}

	if (ctx.message_system)
	{
		ctx.message_system->log("init_tiles: Created " + std::to_string(tiles.size()) + " tiles");
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors, GameContext& ctx)
{
	init_tiles(ctx);
	seed = ctx.dice ? ctx.dice->roll(0, std::numeric_limits<int>::max()) : 0; // for new seed
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	tcodMap = std::make_unique<TCODMap>(map_width, map_height);
	bsp(map_width, map_height, *rng_unique, withActors, ctx);
	tcodPath = std::make_unique<TCODPath>(tcodMap.get(), 1.41f);

	post_process_doors();

	if (ctx.level_manager && ctx.level_manager->get_dungeon_level() > 1) {
		maybe_create_treasure_room(ctx.level_manager->get_dungeon_level(), ctx);
	}
	place_amulet(ctx);

	// DEBUG: Spawn fire and ice wolves in player's starting room for testing fire/cold damage
	if (ctx.level_manager && ctx.level_manager->get_dungeon_level() == 1 && ctx.rooms && ctx.rooms->size() >= 2)
	{
		// Get first room bounds (rooms are stored as begin/end pairs)
		Vector2D roomBegin = (*ctx.rooms)[0];
		Vector2D roomEnd = (*ctx.rooms)[1];

		Vector2D spawnPos1{ 0, 0 };
		Vector2D spawnPos2{ 0, 0 };

		// Find valid positions within the first room
		for (int attempt = 0; attempt < 100; ++attempt)
		{
			int x = ctx.dice->roll(roomBegin.x + 1, roomEnd.x - 1);
			int y = ctx.dice->roll(roomBegin.y + 1, roomEnd.y - 1);
			Vector2D testPos{ y, x };

			if (get_tile_type(testPos) == TileType::FLOOR && !get_actor(testPos, ctx))
			{
				if (spawnPos1.x == 0 && spawnPos1.y == 0)
				{
					spawnPos1 = testPos;
				}
				else if (spawnPos2.x == 0 && spawnPos2.y == 0)
				{
					spawnPos2 = testPos;
					break;
				}
			}
		}

		if (spawnPos1.x != 0 || spawnPos1.y != 0)
		{
			ctx.creatures->push_back(MonsterCreator::create(spawnPos1, MonsterId::FIRE_WOLF, ctx));
			ctx.message_system->log("DEBUG: Spawned fire wolf in starting room at " + std::to_string(spawnPos1.x) + "," + std::to_string(spawnPos1.y));
		}

		if (spawnPos2.x != 0 || spawnPos2.y != 0)
		{
			ctx.creatures->push_back(MonsterCreator::create(spawnPos2, MonsterId::ICE_WOLF, ctx));
			ctx.message_system->log("DEBUG: Spawned ice wolf in starting room at " + std::to_string(spawnPos2.x) + "," + std::to_string(spawnPos2.y));
		}
	}
}

void Map::bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors, GameContext& ctx)
{
	float randomRatio = ctx.dice ? ctx.dice->d100() : 50.0f;
	TCODBsp myBSP(0, 0, map_width, map_height);
	myBSP.splitRecursive(&rng_unique, 4, ROOM_HORIZONTAL_MAX_SIZE, ROOM_VERTICAL_MAX_SIZE, randomRatio, randomRatio);

	BspListener mylistener(*this, ctx);
	myBSP.traverseInvertedLevelOrder(&mylistener, (void*)withActors);
	place_stairs(ctx);
}

void Map::load(const json& j)
{
	map_width = j.at("map_width").get<int>();
	map_height = j.at("map_height").get<int>();
	seed = j.at("seed").get<int>();

	tiles.clear();
	for (const auto& tileJson : j.at("tiles"))
	{
		Vector2D position(
			tileJson.at("position").at("y").get<int>(),
			tileJson.at("position").at("x").get<int>()
		);

		TileType type = static_cast<TileType>(tileJson.at("type").get<int>());
		bool explored = tileJson.at("explored").get<bool>();
		double cost = tileJson.at("cost").get<double>();

		tiles.emplace_back(position, type, cost);
		tiles.back().explored = explored;
	}

	tcodMap = std::make_unique<TCODMap>(map_width, map_height);
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	
	// CRITICAL FOV FIX: Set tcodMap properties based on loaded tile data
	// instead of regenerating via bsp() which would override our loaded tiles
	for (const auto& tile : tiles)
	{
		bool walkable = false;
		bool transparent = false;
		
		switch (tile.type)
		{
		case TileType::FLOOR:
		case TileType::CORRIDOR:
		case TileType::OPEN_DOOR:
			walkable = true;
			transparent = true;
			break;
		case TileType::WATER:
			walkable = true;  // Can walk if have swim ability, but always transparent
			transparent = true;
			break;
		case TileType::WALL:
		case TileType::CLOSED_DOOR:
			walkable = false;
			transparent = false;
			break;
		default:
			walkable = false;
			transparent = false;
			break;
		}
		
		tcodMap->setProperties(tile.position.x, tile.position.y, walkable, transparent);
	}
	
	tcodPath = std::make_unique<TCODPath>(tcodMap.get(), 1.41f);

	// Post-process to place doors after loading map
	post_process_doors();

	// Note: Logging requires GameContext access - moved to caller
}

void Map::save(json& j)
{
	j["map_width"] = map_width;
	j["map_height"] = map_height;
	j["seed"] = seed;

	j["tiles"] = json::array();
	for (const auto& tile : tiles)
	{
		j["tiles"].push_back(
			{
				{"position", { {"y", tile.position.y}, {"x", tile.position.x} } },
				{"type", static_cast<int>(tile.type)}, // TileType is an enum
				{"explored", tile.explored},
				{ "cost", tile.cost }
			}
		);
	}
}

bool Map::is_wall(Vector2D pos) const noexcept
{
	return !tcodMap->isWalkable(pos.x, pos.y);
}

bool Map::is_explored(Vector2D pos) const noexcept
{
	if (!in_bounds(pos)) {
		// Logging moved to callers with GameContext access
		return false;
	}
	size_t index = get_index(pos);
	if (index >= tiles.size()) {
		// Logging moved to callers with GameContext access
		return false;
	}
	return tiles.at(index).explored;
}

void Map::set_explored(Vector2D pos)
{
	if (!in_bounds(pos)) {
		return; // Can't set explored for out of bounds positions
	}
	tiles.at(get_index(pos)).explored = true;
}

bool Map::is_in_fov(Vector2D pos) const noexcept
{
	return tcodMap->isInFov(pos.x, pos.y);
}

bool Map::is_water(Vector2D pos) const noexcept
{
	if (!in_bounds(pos)) {
		return false; // Out of bounds positions are not water
	}
	return tiles.at(get_index(pos)).type == TileType::WATER;
}

TileType Map::get_tile_type(Vector2D pos) const noexcept
{
	if (!in_bounds(pos)) {
		return TileType::WALL; // Out of bounds positions are treated as walls
	}
	return tiles.at(get_index(pos)).type;
}

// returns true if the player after a successful move
void Map::tile_action(Creature& owner, TileType tileType, GameContext& ctx)
{
	switch (tileType)
	{
	case TileType::WATER:
		// Only called on successful water entry (with swim ability)
		if (ctx.message_system)
		{
			ctx.message_system->log("You are in water");
			ctx.message_system->message(COLOR_WHITE, "You are in water", true);
		}
		break;
	case TileType::WALL:
		if (ctx.message_system)
		{
			ctx.message_system->log("You are against a wall");
			ctx.message_system->message(COLOR_WHITE, "You are against a wall", true);
		}
		break;
	case TileType::FLOOR:
		//ctx.message_system->log("You are on the floor");
		//ctx.message_system->message(COLOR_WHITE, "You are on the floor", true);
		break;
	case TileType::CLOSED_DOOR:
		if (ctx.message_system)
		{
			ctx.message_system->log("You are at a door");
			ctx.message_system->message(COLOR_WHITE, "You are at a door", true);
		}
		break;
	case TileType::CORRIDOR:
		//ctx.message_system->log("You are in a corridor");
		//ctx.message_system->message(COLOR_WHITE, "You are in a corridor", true);
		break;
	default:
		if (ctx.message_system)
		{
			ctx.message_system->log("You are in an unknown area");
		}
	}
}

bool Map::is_collision(Creature& owner, TileType tileType, Vector2D pos, GameContext& ctx)
{
	// if there is an actor at the position
	const auto& actor = get_actor(pos, ctx);
	if (actor)
	{
		return true;
	}

	switch (tileType)
	{
	case TileType::WATER:
		/*return owner.has_state(ActorState::CAN_SWIM) ? false : true;*/
		return false;
	case TileType::WALL:
		return true;
	case TileType::FLOOR:
		return false;
	case TileType::CLOSED_DOOR:
		return true;  // Closed doors block movement
	case TileType::OPEN_DOOR:
		return false; // Open doors don't block movement
	case TileType::CORRIDOR:
		return false;
	default:
		return true;
	}
}

void Map::compute_fov(GameContext& ctx)
{
	if (ctx.message_system)
	{
		ctx.message_system->log("...Computing FOV...");
	}
	// Safety check for valid player position
	if (!ctx.player ||
		ctx.player->position.x < 0 || ctx.player->position.x >= map_width ||
		ctx.player->position.y < 0 || ctx.player->position.y >= map_height) {
		if (ctx.message_system)
		{
			ctx.message_system->log("Warning: Can't compute FOV - invalid player position");
		}
		return;
	}
	tcodMap->computeFov(ctx.player->position.x, ctx.player->position.y, FOV_RADIUS, true, FOV_SYMMETRIC_SHADOWCAST);
}

void Map::update()
{
	for (auto& tile : tiles)
	{
		if (is_in_fov(tile.position))
		{
			set_explored(tile.position);
		}
	}
}

void Map::render() const
{
	// Logging moved to callers with GameContext access

	// Don't render if tiles aren't initialized yet
	if (tiles.empty()) {
		return;
	}
	
	for (const auto& tile : tiles)
	{
		if (is_in_fov(tile.position) || is_explored(tile.position))
		{
			switch (get_tile_type(tile.position))
			{
			case TileType::WALL:
				attron(COLOR_PAIR(WHITE_BLACK_PAIR));
				mvaddch(tile.position.y, tile.position.x, '#');
				attroff(COLOR_PAIR(WHITE_BLACK_PAIR));
				break;
			case TileType::WATER:
				attron(COLOR_PAIR(BLUE_BLACK_PAIR));
				mvaddch(tile.position.y, tile.position.x, '~');
				attroff(COLOR_PAIR(BLUE_BLACK_PAIR));
				break;
			case TileType::FLOOR:
				mvaddch(tile.position.y, tile.position.x, '.');
				break;
			case TileType::CLOSED_DOOR:
				attron(COLOR_PAIR(BROWN_BLACK_PAIR));
				mvaddch(tile.position.y, tile.position.x, '+'); // Closed door character
				attroff(COLOR_PAIR(BROWN_BLACK_PAIR));
				break;
			case TileType::OPEN_DOOR:
				attron(COLOR_PAIR(BROWN_BLACK_PAIR));
				mvaddch(tile.position.y, tile.position.x, '/'); // Different symbol for open door
				attroff(COLOR_PAIR(BROWN_BLACK_PAIR));
				break;
			case TileType::CORRIDOR:
				//attron(COLOR_PAIR(WHITE_GREEN_PAIR));
				mvaddch(tile.position.y, tile.position.x, '.');
				//attroff(COLOR_PAIR(WHITE_GREEN_PAIR));
				break;
			default:
				break;
			}
		}
	}
}

void Map::add_item(Vector2D pos, GameContext& ctx) {
	// 75% chance to spawn an item at all
	if (rng_unique->getInt(1, 100) > 75) return;

	// Use our ItemFactory to create a random item
	if (ctx.level_manager)
	{
		itemFactory->spawn_random_item(pos, ctx, ctx.level_manager->get_dungeon_level());
	}
}

void Map::dig(Vector2D begin, Vector2D end)
{
	if (begin.x > end.x) { std::swap(begin.x, end.x); }
	if (begin.y > end.y) { std::swap(begin.y, end.y); }

	//const int rollD2 = game.d.d2(); // 50% to dig square or diamond shape
	const int rollD2 = rng_unique->getInt(1, 2); // 50% to dig square or diamond shape
	if (rollD2 == 1)
	{
		for (int tileY = begin.y; tileY <= end.y; tileY++)
		{
			for (int tileX = begin.x; tileX <= end.x; tileX++)
			{
				set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR, 1);
				tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
			}
		}
	}
	else
	{
		int width = end.x - begin.x + 1;
		int centerX = (begin.x + end.x) / 2;
		int centerY = (begin.y + end.y) / 2;

		for (int tileY = begin.y; tileY <= end.y; tileY++)
		{
			// Proper casting to float for the math
			float verticalDist = static_cast<float>(std::abs(tileY - centerY));
			float verticalRatio = verticalDist / static_cast<float>(centerY);

			// Explicitly cast the final result to int to silence "loss of data" warnings    
			int halfWidth = static_cast<int>(static_cast<float>(width / 2) * (1.0f - verticalRatio));

			int startX = centerX - halfWidth;    
			int endX = centerX + halfWidth;

			for (int tileX = startX; tileX <= endX; tileX++)     
			{
				if (tileX >= begin.x && tileX <= end.x) 
				{
					// Vector2D is {y, x}
					set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR, 1);
					// NOTE: libtcod's setProperties expects (x, y)
					tcodMap->setProperties(tileX, tileY, true, true);
				}
			}
		}
	}
}

void Map::dig_corridor(Vector2D begin, Vector2D end)
{
    // Ensure begin.x <= end.x and begin.y <= end.y for consistent loops
    // Original code did this effectively, keeping for consistency.
    int x1 = begin.x;
    int y1 = begin.y;
    int x2 = end.x;
    int y2 = end.y;

    // --- CRITICAL FIX 1: Dig 1-tile wide L-shaped corridor ---
    // This reinterpretation makes "perfect alignment" much more achievable.
    // Choose a random path direction (horizontal first or vertical first)
    // Note: Random generation uses internal rng_unique, not GameContext
    bool horizontal_first = rng_unique->getInt(0, 1) == 1;

    Vector2D current_pos = begin; // Start digging from 'begin'

    if (horizontal_first) {
        // Dig horizontally first
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
            current_pos = Vector2D{y1, x};
            set_tile(current_pos, TileType::CORRIDOR, 1);
            tcodMap->setProperties(current_pos.x, current_pos.y, true, true);
        }
        // Then dig vertically
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
            if (y == y1) continue; // Skip corner - already dug horizontally
            current_pos = Vector2D{y, x2};
            set_tile(current_pos, TileType::CORRIDOR, 1);
            tcodMap->setProperties(current_pos.x, current_pos.y, true, true);
        }
    } else {
        // Dig vertically first
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
            current_pos = Vector2D{y, x1};
            set_tile(current_pos, TileType::CORRIDOR, 1);
            tcodMap->setProperties(current_pos.x, current_pos.y, true, true);
        }
        // Then dig horizontally
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
            if (x == x1) continue; // Skip corner - already dug vertically
            current_pos = Vector2D{y2, x};
            set_tile(current_pos, TileType::CORRIDOR, 1);
            tcodMap->setProperties(current_pos.x, current_pos.y, true, true);
        }
    }
}

void Map::set_door(Vector2D thisTile, int tileX, int tileY)
{
	set_tile(thisTile, TileType::CLOSED_DOOR, 2);
	tcodMap->setProperties(tileX, tileY, false, false);
}

void Map::set_tile(Vector2D pos, TileType newType, double cost)
{
	if (!in_bounds(pos)) {
		return; // Can't set tile for out of bounds positions
	}
	tiles.at(get_index(pos)).type = newType;
	tiles.at(get_index(pos)).cost = cost;
}

void Map::create_room(bool first, int x1, int y1, int x2, int y2, bool withActors, GameContext& ctx)
{
	Vector2D begin{ y1,x1 };
	Vector2D end{ y2,x2 };

	// Store room coordinates via GameContext
	if (ctx.rooms)
	{
		ctx.rooms->emplace_back(begin);
		ctx.rooms->emplace_back(end);
	}

	dig(begin, end); // dig the corridors

	spawn_water(begin, end, ctx);

	if (!withActors)
	{
		return;
	}

	if (first) // if this is the first room, we need to place the player in it
	{
		spawn_player(begin, end, ctx);
	}

	spawn_items(begin, end, ctx);

	// No monster spawning during room creation - monsters spawn procedurally during gameplay
	// This was the original design - only items spawn during level generation
}

void Map::spawn_water(Vector2D begin, Vector2D end, GameContext& ctx)
{
	// Add water tiles
	constexpr int waterPercentage = 5; // 5% of tiles will be water (reduced from 10%)
	Vector2D waterPos{ 0,0 };
	for (waterPos.y = begin.y; waterPos.y <= end.y; ++waterPos.y)
	{
		for (waterPos.x = begin.x; waterPos.x <= end.x; ++waterPos.x)
		{
			/*const int rolld100 = game.d.d100();*/
			const int rolld100 = rng_unique->getInt(1, 100);
			if (rolld100 < waterPercentage)
			{
				// CRITICAL FIX: Check if water would block entrances using pattern matching
				if (!would_water_block_entrance(waterPos, ctx)) {
					set_tile(waterPos, TileType::WATER, 10);
					tcodMap->setProperties(waterPos.x, waterPos.y, true, true); // non-walkable and non-transparent
				}
			}
		}
	}
}

bool Map::would_water_block_entrance(Vector2D waterPos, GameContext& ctx) const
{
	// Pattern matching to prevent water from blocking entrances
	// Check all 8 directions around the water position for potential entrance patterns
	
	// Get surrounding positions
	std::vector<Vector2D> adjacent = {
		{waterPos.y - 1, waterPos.x - 1}, {waterPos.y - 1, waterPos.x}, {waterPos.y - 1, waterPos.x + 1}, // Top row
		{waterPos.y, waterPos.x - 1},                                   {waterPos.y, waterPos.x + 1},     // Middle row (excluding center)
		{waterPos.y + 1, waterPos.x - 1}, {waterPos.y + 1, waterPos.x}, {waterPos.y + 1, waterPos.x + 1}  // Bottom row
	};
	
	// Count walls and floors around this position
	int wallCount = 0;
	int floorCount = 0;
	
	for (const auto& pos : adjacent) {
		if (!in_bounds(pos)) {
			wallCount++; // Out of bounds = wall
			continue;
		}
		
		TileType tileType = get_tile_type(pos);
		if (tileType == TileType::WALL) {
			wallCount++;
		} else if (tileType == TileType::FLOOR) {
			floorCount++;
		}
	}
	
	// ENTRANCE PATTERN 1: Corner or edge position (high wall density)
	// If surrounded by 5+ walls, this is likely a corner or edge - safe for water
	if (wallCount >= 5) {
		return false; // Safe to place water
	}
	
	// ENTRANCE PATTERN 2: Doorway position (walls on opposite sides)
	// Check for corridor-like patterns: walls on opposite sides
	auto isWallOrOOB = [this](Vector2D pos) {
		return !in_bounds(pos) || get_tile_type(pos) == TileType::WALL;
	};
	
	// Check horizontal corridor pattern: W-F-W (wall-floor-wall)
	if (isWallOrOOB({waterPos.y, waterPos.x - 1}) && isWallOrOOB({waterPos.y, waterPos.x + 1})) {
		return true; // Would block horizontal passage
	}
	
	// Check vertical corridor pattern: W
	//                                    F  
	//                                    W
	if (isWallOrOOB({waterPos.y - 1, waterPos.x}) && isWallOrOOB({waterPos.y + 1, waterPos.x})) {
		return true; // Would block vertical passage
	}
	
	// ENTRANCE PATTERN 3: Near room edge with potential door placement
	// Check if this position could be part of a future door placement
	// Look for patterns where this floor tile is adjacent to potential door areas
	
	// Check for L-shaped entrance patterns (common near room corners)
	int adjacentWalls = 0;
	int adjacentFloors = 0;
	
	// Count direct adjacent (not diagonal) walls and floors
	std::vector<Vector2D> directAdjacent = {
		{waterPos.y - 1, waterPos.x}, // North
		{waterPos.y + 1, waterPos.x}, // South  
		{waterPos.y, waterPos.x - 1}, // West
		{waterPos.y, waterPos.x + 1}  // East
	};
	
	for (const auto& pos : directAdjacent) {
		if (!in_bounds(pos)) {
			adjacentWalls++;
			continue;
		}
		
		TileType tileType = get_tile_type(pos);
		if (tileType == TileType::WALL) {
			adjacentWalls++;
		} else if (tileType == TileType::FLOOR) {
			adjacentFloors++;
		}
	}
	
	// ENTRANCE PATTERN 4: Potential door position
	// If this position has exactly 2 walls and 2 floors adjacent, it might be a door spot
	if (adjacentWalls == 2 && adjacentFloors == 2) {
		// Check if walls are opposite each other (forming a corridor)
		bool hasOppositeWalls = 
			(isWallOrOOB({waterPos.y - 1, waterPos.x}) && isWallOrOOB({waterPos.y + 1, waterPos.x})) ||
			(isWallOrOOB({waterPos.y, waterPos.x - 1}) && isWallOrOOB({waterPos.y, waterPos.x + 1}));
		
		if (hasOppositeWalls) {
			return true; // Would block potential door
		}
	}
	
	// ENTRANCE PATTERN 5: Room boundary positions
	// If this floor tile is on the edge of the room, it might be where a corridor connects
	Vector2D roomBegin = {0, 0};
	Vector2D roomEnd = {0, 0};

	// Find which room this position belongs to
	if (ctx.rooms)
	{
		for (size_t i = 0; i < ctx.rooms->size(); i += 2) {
			if (i + 1 < ctx.rooms->size()) {
				Vector2D begin = (*ctx.rooms)[i];
				Vector2D end = (*ctx.rooms)[i + 1];

				if (waterPos.y >= begin.y && waterPos.y <= end.y &&
					waterPos.x >= begin.x && waterPos.x <= end.x) {
					roomBegin = begin;
					roomEnd = end;
					break;
				}
			}
		}
	}
	
	// Check if this position is on the room perimeter (edge positions are potential entrance points)
	if (waterPos.y == roomBegin.y || waterPos.y == roomEnd.y ||
		waterPos.x == roomBegin.x || waterPos.x == roomEnd.x) {
		// This is on room edge - higher chance of being an entrance, so be more cautious
		if (adjacentWalls >= 1 && adjacentFloors >= 1) {
			return true; // Potentially blocks room entrance
		}
	}
	
	// If none of the blocking patterns match, water is safe to place
	return false;
}

void Map::spawn_items(Vector2D begin, Vector2D end, GameContext& ctx)
{
	// add items
	const int numItems = ctx.dice ? ctx.dice->roll(0, MAX_ROOM_ITEMS) : 0;
	for (int i = 0; i < numItems; i++)
	{
		Vector2D itemPos{ ctx.dice ? ctx.dice->roll(begin.y, end.y) : begin.y, ctx.dice ? ctx.dice->roll(begin.x, end.x) : begin.x };
		while (!can_walk(itemPos, ctx) || is_stairs(itemPos, ctx))
		{
			itemPos.x = ctx.dice ? ctx.dice->roll(begin.x, end.x) : begin.x;
			itemPos.y = ctx.dice ? ctx.dice->roll(begin.y, end.y) : begin.y;
		}
		add_item(itemPos, ctx);
	}
}

void Map::spawn_player(Vector2D begin, Vector2D end, GameContext& ctx)
{
	// add player
	if (!ctx.player || !ctx.dice) return;

	Vector2D playerPos{ ctx.dice->roll(begin.y, end.y), ctx.dice->roll(begin.x, end.x) };
	while (!can_walk(playerPos, ctx))
	{
		playerPos.x = ctx.dice->roll(begin.x, end.x);
		playerPos.y = ctx.dice->roll(begin.y, end.y);
	}
	ctx.player->position.x = playerPos.x;
	ctx.player->position.y = playerPos.y;
}

void Map::place_stairs(GameContext& ctx)
{
	if (!ctx.stairs || !ctx.rooms || ctx.rooms->empty() || !ctx.dice) return;

	int index = ctx.dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1);
	index = index % 2 == 0 ? index : index - 1;
	const Vector2D roomBegin = ctx.rooms->at(index);
	const Vector2D roomEnd = ctx.rooms->at(index + 1);
	Vector2D stairsPos{ ctx.dice->roll(roomBegin.y, roomEnd.y), ctx.dice->roll(roomBegin.x, roomEnd.x) };
	while (!can_walk(stairsPos, ctx))
	{
		stairsPos.x = ctx.dice->roll(roomBegin.x, roomEnd.x);
		stairsPos.y = ctx.dice->roll(roomBegin.y, roomEnd.y);
	}
	ctx.stairs->position.x = stairsPos.x;
	ctx.stairs->position.y = stairsPos.y;
}

bool Map::is_stairs(Vector2D pos, GameContext& ctx) const
{
	return ctx.stairs && ctx.stairs->position == pos;
}

bool Map::can_walk(Vector2D pos, GameContext& ctx) const noexcept
{
    if (is_wall(pos)) // check if the tile is a wall
    {
        return false;
    }

    // Check for doors - can only walk through open doors
    if (get_tile_type(pos) == TileType::CLOSED_DOOR)
    {
        return false; // Closed doors block movement
    }

    // CRITICAL FIX: Check if there's already an actor at this position
    if (get_actor(pos, ctx) != nullptr)
    {
        return false; // Position occupied by another actor
    }

    return true;
}

void Map::add_monster(Vector2D pos, GameContext& ctx) const
{
	// Use the monster factory to create a monster appropriate for the current dungeon level
	if (ctx.level_manager)
	{
		monsterFactory->spawn_random_monster(pos, ctx.level_manager->get_dungeon_level(), ctx);

		// Log the spawn for debugging
		Creature* monster = get_actor(pos, ctx);
		if (monster && ctx.message_system)
		{
			ctx.message_system->log("Spawned " + monster->actorData.name + " at level " + std::to_string(ctx.level_manager->get_dungeon_level()));
		}
	}
}

// getActor returns the actor at the given coordinates or `nullptr` if there's none
Creature* Map::get_actor(Vector2D pos, GameContext& ctx) const noexcept
{
	// Check player position first
	if (ctx.player && ctx.player->position == pos)
	{
		return ctx.player;
	}

	// Check all creatures (excluding dead ones - they shouldn't block)
	for (const auto& actor : *ctx.creatures)
	{
		if (actor && actor->position == pos)
		{
			// Only return living creatures as blockers
			if (actor->destructible && !actor->destructible->is_dead())
			{
				return actor.get();
			}
		}
	}

	return nullptr;
}

std::vector<std::vector<Tile>> Map::get_map() const noexcept
{
	std::vector<std::vector<Tile>> map;
	for (const auto& tile : tiles)
	{
		map.push_back({ tile });
	}
	return map;
}

// reveal map
void Map::reveal()
{
	for (auto& tile : tiles)
	{
		tile.explored = true;
	}
}

// regenerate map
void Map::regenerate(GameContext& ctx)
{
	clear();
	refresh();
	// clear the actors container except the player and the stairs
	if (ctx.creatures)
	{
		ctx.creatures->clear();
	}
	if (ctx.inventory_data)
	{
		ctx.inventory_data->items.clear();
	}
	if (ctx.rooms)
	{
		ctx.rooms->clear(); // we clear the room coordinates
	}
	if (ctx.objects)
	{
		ctx.objects->clear();
	}

	// generate a new map
	map_height = MAP_HEIGHT;
	map_width = MAP_WIDTH;
	init(true, ctx);
}

std::vector<Vector2D> Map::neighbors(Vector2D id, GameContext& ctx, Vector2D target)
{
	std::vector<Vector2D> results;

	for (Vector2D dir : DIRS)
	{
		Vector2D next{ id.y + dir.y, id.x + dir.x };
		if (in_bounds(next))
		{
			// Allow target position even if occupied (for pathfinding to reach goal)
			bool isTarget = (target.x != -1 && next == target);
			if (isTarget)
			{
				// For target, only check terrain (walls, doors) not actors
				if (!is_wall(next) && get_tile_type(next) != TileType::CLOSED_DOOR)
				{
					results.push_back(next);
				}
			}
			else if (can_walk(next, ctx))
			{
				results.push_back(next);
			}
		}
	}

	if ((id.x + id.y) % 2 == 0)
	{
		// see "Ugly paths" section for an explanation:
		std::reverse(results.begin(), results.end());
	}

	return results;
}

double Map::cost(Vector2D from_node, Vector2D to_node, GameContext& ctx)
{
	// if there is an actor on the tile, return a high cost
	if (get_actor(to_node, ctx) != nullptr)
	{
		return 1000.0;
	}
	return get_cost(to_node, ctx);
}

double Map::get_cost(Vector2D pos, GameContext& ctx) const noexcept
{
	if (!in_bounds(pos)) {
		// Logging moved to callers with GameContext access
		return 1000.0; // High cost for out of bounds
	}
	size_t index = get_index(pos);
	if (index >= tiles.size()) {
		// Logging moved to callers with GameContext access
		return 1000.0;
	}
	return tiles.at(index).cost;
}

// TODO: why implement bresenham zillion times?
bool Map::has_los(Vector2D from, Vector2D to) const noexcept
{
	int x0 = from.x;
	int y0 = from.y;
	int x1 = to.x;
	int y1 = to.y;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (x0 != x1 || y0 != y1) {
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}

		// Skip the start and end points
		if ((x0 != from.x || y0 != from.y) && (x0 != to.x || y0 != to.y))
		{
			if (is_wall(Vector2D{ y0, x0 }))
			{
				return false;
			}
		}
	}

	return true;
}

bool Map::is_door(Vector2D pos) const noexcept
{
	if (pos.y < 0 || pos.y >= map_height || pos.x < 0 || pos.x >= map_width) // TODO: i think i saw this logic before? DRY violation.
		return false;

	TileType tileType = get_tile_type(pos);
	return tileType == TileType::CLOSED_DOOR || tileType == TileType::OPEN_DOOR;
}

bool Map::is_open_door(Vector2D pos) const noexcept
{
	if (pos.y < 0 || pos.y >= map_height || pos.x < 0 || pos.x >= map_width)
		return false;
	return get_tile_type(pos) == TileType::OPEN_DOOR;
}

bool Map::open_door(Vector2D pos, GameContext& ctx)
{
	if (!is_door(pos))
	{
		return false;
	}

	// Check if the door is already open
	if (get_tile_type(pos) == TileType::OPEN_DOOR)
	{
		return false;
	}

	set_tile(pos, TileType::OPEN_DOOR, 1);
	tcodMap->setProperties(pos.x, pos.y, true, true);

	if (ctx.player && ctx.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov(ctx);
	}

	return true;
}

bool Map::close_door(Vector2D pos, GameContext& ctx)
{
	if (!is_door(pos))
	{
		return false;
	}

	// Check if the door is already closed
	if (get_tile_type(pos) == TileType::CLOSED_DOOR)
	{
		return false;
	}

	// Check if there's an actor on the door - can't close if occupied
	if (get_actor(pos, ctx) != nullptr)
	{
		return false;
	}

	// Change the tile type to DOOR (closed)
	set_tile(pos, TileType::CLOSED_DOOR, 2);

	// Make the tile non-walkable and non-transparent
	tcodMap->setProperties(pos.x, pos.y, false, false);

	if (ctx.player && ctx.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov(ctx);
	}

	return true;
}

void Map::place_amulet(GameContext& ctx)
{
	// Only place the amulet on the final level
	if (!ctx.level_manager || !ctx.player || !ctx.rooms || !ctx.dice) return;

	if (ctx.level_manager->get_dungeon_level() == FINAL_DUNGEON_LEVEL)
	{
		// Choose a random room for the amulet
		int index = ctx.dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1);
		index = index % 2 == 0 ? index : index - 1;
		const Vector2D roomBegin = ctx.rooms->at(index);
		const Vector2D roomEnd = ctx.rooms->at(index + 1);

		// Find a walkable position in the room
		Vector2D amuletPos{ ctx.dice->roll(roomBegin.y, roomEnd.y), ctx.dice->roll(roomBegin.x, roomEnd.x) };
		while (!can_walk(amuletPos, ctx) || is_stairs(amuletPos, ctx))
		{
			amuletPos.x = ctx.dice->roll(roomBegin.x, roomEnd.x);
			amuletPos.y = ctx.dice->roll(roomBegin.y, roomEnd.y);
		}

		// Create and place the amulet
		InventoryOperations::add_item(ctx.player->inventory_data, ItemCreator::create(ItemId::AMULET_OF_YENDOR, amuletPos));

		// Log the placement (debug info)
		if (ctx.message_system)
		{
			ctx.message_system->log("Placed Amulet of Yendor at " + std::to_string(amuletPos.x) + "," + std::to_string(amuletPos.y));

			// Add a hint message
			ctx.message_system->message(RED_YELLOW_PAIR, "You sense a powerful artifact somewhere on this level...", true);
		}
	}
}

// TODO: wohoo ! i found this very useful function finally. SHOULD IT BE IN MAP?!
void Map::display_spawn_rates(GameContext& ctx) const
{
	if (!ctx.level_manager) return;

	WINDOW* ratesWindow = newwin(
		24,  // height (adjust to fit all monsters)
		50,  // width
		1,   // y position
		1    // x position
	);

	box(ratesWindow, 0, 0);
	mvwprintw(ratesWindow, 1, 1, "Monster Spawn Rates (Dungeon Level %d)", ctx.level_manager->get_dungeon_level());
	mvwprintw(ratesWindow, 2, 1, "--------------------------------------");

	// Get current distribution from monster factory
	auto distribution = monsterFactory->getCurrentDistribution(ctx.level_manager->get_dungeon_level());

	// Sort by probability (descending)
	std::sort(distribution.begin(), distribution.end(),
		[](const auto& a, const auto& b) { return a.second > b.second; });

	int row = 3;
	for (const auto& [name, percentage] : distribution) {
		mvwprintw(ratesWindow, row++, 1, "%-15s: %5.1f%%", name.c_str(), percentage);
	}

	mvwprintw(ratesWindow, row + 1, 1, "Press any key to close");
	wrefresh(ratesWindow);
	getch();  // Wait for key press
	delwin(ratesWindow);
	clear();
}

void Map::create_treasure_room(Vector2D begin, Vector2D end, int quality, GameContext& ctx)
{
	if (!ctx.level_manager || !ctx.dice) return;

	// Mark the area for the treasure room
	for (int y = begin.y; y <= end.y; y++)
	{
		for (int x = begin.x; x <= end.x; x++)
		{
			set_tile(Vector2D{ y, x }, TileType::FLOOR, 1);
			tcodMap->setProperties(x, y, true, true);
		}
	}

	// Add some decorative elements (different floor tiles or room features)
	// Here we're just using special markings for demonstration
	for (int y = begin.y; y <= end.y; y++)
	{
		for (int x = begin.x; x <= end.x; x++)
		{
			// Add random decorative elements
			if (ctx.dice->d100() <= 20) {
				// This could be replaced with actual decorative elements
				// Currently just marking for demonstration
			}
		}
	}

	// Calculate the center of the room
	Vector2D center = {
		begin.y + (end.y - begin.y) / 2,
		begin.x + (end.x - begin.x) / 2
	};

	// Generate treasure at the center of the room
	itemFactory->generate_treasure(center, ctx, ctx.level_manager->get_dungeon_level(), quality);

	// Add guardians or traps based on quality
	int guardianCount = 0;
	switch (quality)
	{
	case 1:
	{
		guardianCount = ctx.dice->roll(0, 1);
		break;
	}
	case 2:
	{
		guardianCount = ctx.dice->roll(1, 2);
		break;
	}
	case 3:
	{
		guardianCount = ctx.dice->roll(2, 3);
		break;
	}
	}

	for (int i = 0; i < guardianCount; i++)
	{
		Vector2D guardPos;
		do
		{
			guardPos.y = ctx.dice->roll(begin.y, end.y);
			guardPos.x = ctx.dice->roll(begin.x, end.x);
			// Ensure we don't place on the center (where treasure is)
			// and the position is valid
		} while ((guardPos.y == center.y && guardPos.x == center.x) ||
			!can_walk(guardPos, ctx) ||
			get_actor(guardPos, ctx) != nullptr);

		// Create a guardian appropriate for the treasure quality
		add_monster(guardPos, ctx);
	}

	if (ctx.message_system)
	{
		ctx.message_system->log("Created treasure room at (" + std::to_string(begin.x) + "," +
			std::to_string(begin.y) + ") to (" + std::to_string(end.x) + "," +
			std::to_string(end.y) + ") with quality " + std::to_string(quality));
	}
}

bool Map::maybe_create_treasure_room(int dungeonLevel, GameContext& ctx)
{
	if (!ctx.dice || !ctx.rooms) return false;

	// Probability increases with dungeon level
	int treasureRoomChance = 5 + (dungeonLevel * 2); // 5% + 2% per level

	// Cap at 25%
	treasureRoomChance = std::min(treasureRoomChance, 25);

	if (ctx.dice->d100() > treasureRoomChance)
	{
		return false; // No treasure room this time
	}

	// Find a suitable room from the rooms list
	if (ctx.rooms->size() < 2)
	{
		return false; // Need at least one room (which is 2 entries in the vector)
	}

	// Select a random room (skipping the first room, which usually has the player)
	int index = ctx.dice->roll(2, static_cast<int>(ctx.rooms->size() - 1));
	index = index % 2 == 0 ? index : index - 1; // Ensure it's even (room starts)

	if (index >= static_cast<int>(ctx.rooms->size()))
	{
		return false; // Safety check
	}

	const Vector2D roomBegin = ctx.rooms->at(index);
	const Vector2D roomEnd = ctx.rooms->at(index + 1);

	// Calculate room size
	int width = roomEnd.x - roomBegin.x;
	int height = roomEnd.y - roomBegin.y;

	// Only use rooms that are large enough
	if (width < 6 || height < 6)
	{
		return false;
	}

	// Determine the quality of the treasure room based on dungeon level and luck
	int quality = 1; // Default is normal

	int qualityRoll = ctx.dice->d100();
	if (qualityRoll <= 5 + dungeonLevel)
	{
		quality = 3; // Exceptional (5% + dungeonLevel%)
	}
	else if (qualityRoll <= 15 + (dungeonLevel * 2))
	{
		quality = 2; // Good (15% + dungeonLevel*2%)
	}

	// Create the treasure room
	create_treasure_room(roomBegin, roomEnd, quality, ctx);

	return true;
}

// TODO: found another useful function! I like these but i could not find them and use them before.
void Map::display_item_distribution(GameContext& ctx) const
{
	if (!ctx.level_manager) return;

	WINDOW* distributionWindow = newwin(
		24,  // height (adjust to fit all items)
		50,  // width
		1,   // y position
		1    // x position
	);

	box(distributionWindow, 0, 0);
	mvwprintw(distributionWindow, 1, 1, "Item Spawn Rates (Dungeon Level %d)", ctx.level_manager->get_dungeon_level());
	mvwprintw(distributionWindow, 2, 1, "--------------------------------------");

	// Get current distribution from item factory
	std::vector<ItemPercentage> distribution = itemFactory->get_current_distribution(ctx.level_manager->get_dungeon_level());

	// Sort by probability (descending)
	std::sort(distribution.begin(), distribution.end(),
		[](const auto& a, const auto& b) { return a.percentage > b.percentage; });

	int row = 3;

	// First display categories
	std::unordered_map<std::string, float> categoryTotals;

	for (const auto& [name, percentage] : distribution)
	{
		std::string category;

		if (name.find("Potion") != std::string::npos)
		{
			category = "Potions";
		}
		else if (name.find("Scroll") != std::string::npos)
		{
			category = "Scrolls";
		}
		else if (name == "Gold")
		{
			category = "Gold";
		}
		else if (name == "Amulet of Yendor")
		{
			category = "Artifacts";
		}
		else if (name.find("Ration") != std::string::npos ||
			name.find("Fruit") != std::string::npos ||
			name.find("Bread") != std::string::npos ||
			name.find("Meat") != std::string::npos
			)
		{
			category = "Food";
		}
		else
		{
			category = "Weapons";
		}

		categoryTotals[category] += percentage;
	}

	mvwprintw(distributionWindow, row++, 1, "Item Categories:");
	for (const auto& [category, percentage] : categoryTotals)
	{
		mvwprintw(distributionWindow, row++, 1, "%-15s: %5.1f%%", category.c_str(), percentage);
	}

	row++;
	mvwprintw(distributionWindow, row++, 1, "Individual Items:");

	// Then list individual items
	for (const auto& [name, percentage] : distribution)
	{
		mvwprintw(distributionWindow, row++, 1, "%-20s: %5.1f%%", name.c_str(), percentage);
	}

	mvwprintw(distributionWindow, row + 1, 1, "Press any key to close");
	wrefresh(distributionWindow);
	getch();  // Wait for key press
	delwin(distributionWindow);
	clear();
}

void Map::post_process_doors()
{
    auto isRoom = [this](Vector2D pos) { return in_bounds(pos) && (get_tile_type(pos) == TileType::FLOOR || get_tile_type(pos) == TileType::WATER); };
    auto isWall = [this](Vector2D pos) { return in_bounds(pos) && (get_tile_type(pos) == TileType::WALL || get_tile_type(pos) == TileType::WATER); };

    for (int y = 0; y < map_height; ++y)
    {
        for (int x = 0; x < map_width; ++x)
        {
            Vector2D pos{ y, x };
            if (get_tile_type(pos) == TileType::CORRIDOR)
            {
                int roomNeighbors = 0;
                int wallNeighbors = 0;
                
                for (Vector2D dir : {Vector2D{-1, 0}, Vector2D{1, 0}, Vector2D{0, -1}, Vector2D{0, 1}})
                {
                    Vector2D neighborPos = {pos.y + dir.y, pos.x + dir.x};
                    if (!in_bounds(neighborPos)) continue;
                    
                    if (isRoom(neighborPos))
                    {
                        roomNeighbors++;
                    }
                    else if (isWall(neighborPos))
                    {
                        wallNeighbors++;
                    }
                }
                
                if (roomNeighbors >= 1 && wallNeighbors >= 2)
                {
                    // Get 3x3 grid around current position
                    Vector2D upLeft = {pos.y - 1, pos.x - 1};
                    Vector2D up = {pos.y - 1, pos.x};
                    Vector2D upRight = {pos.y - 1, pos.x + 1};
                    Vector2D left = {pos.y, pos.x - 1};
                    Vector2D right = {pos.y, pos.x + 1};
                    Vector2D downLeft = {pos.y + 1, pos.x - 1};
                    Vector2D down = {pos.y + 1, pos.x};
                    Vector2D downRight = {pos.y + 1, pos.x + 1};
                    
                    // Exclude pattern RRR/WCC/WWW (and rotations)
                    bool shouldExclude = false;
                    
                    // Original: RRR/WCC/WWW
                    if (isRoom(upLeft) && isRoom(up) && isRoom(upRight) &&
                        isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
                        isWall(downLeft) && isWall(down) && isWall(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // 90� rotation: WWR/CWR/CWR
                    if (isWall(upLeft) && isWall(up) && isRoom(upRight) &&
                        get_tile_type(left) == TileType::CORRIDOR && isRoom(right) &&
                        get_tile_type(downLeft) == TileType::CORRIDOR && isWall(down) && isRoom(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // 180� rotation: WWW/CCW/RRR
                    if (isWall(upLeft) && isWall(up) && isWall(upRight) &&
                        get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
                        isRoom(downLeft) && isRoom(down) && isRoom(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // 270� rotation: RWC/RWC/RWW
                    if (isRoom(upLeft) && isWall(up) && get_tile_type(upRight) == TileType::CORRIDOR &&
                        isRoom(left) && get_tile_type(right) == TileType::CORRIDOR &&
                        isRoom(downLeft) && isWall(down) && isWall(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // Exclude WRR/WCC/WWC pattern
                    if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
                        isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
                        isWall(downLeft) && isWall(down) && get_tile_type(downRight) == TileType::CORRIDOR)
                    {
                        shouldExclude = true;
                    }
                    
                    // Exclude WRw/WCC/WWW pattern (where w = water)
                    if (isWall(upLeft) && isRoom(up) && get_tile_type(upRight) == TileType::WATER &&
                        isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
                        isWall(downLeft) && isWall(down) && isWall(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    if (shouldExclude)
                    {
                        continue; // Skip placing door for excluded patterns
                    }
                    
                    // Check for WCW/RDW/RWW pattern (move door UP)
                    if (isWall(upLeft) && get_tile_type(up) == TileType::CORRIDOR && isWall(upRight) &&
                        isRoom(left) && isWall(right) &&
                        isRoom(downLeft) && isWall(down) && isWall(downRight))
                    {
                        // Move door UP
                        Vector2D upPos = {pos.y - 1, pos.x};
                        set_door(upPos, upPos.x, upPos.y);
                    }
                    // Check for W.w/CDW/WWW pattern (move door UP) - where . = water or corridor
                    else if (isWall(upLeft) && 
                        (get_tile_type(up) == TileType::WATER || get_tile_type(up) == TileType::CORRIDOR) &&
                        get_tile_type(upRight) == TileType::WATER &&
                        get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
                        isWall(downLeft) && isWall(down) && isWall(downRight))
                    {
                        // Move door UP
                        Vector2D upPos = {pos.y - 1, pos.x};
                        set_door(upPos, upPos.x, upPos.y);
                    }
                    // Check for Z-pattern: WRR/CCW/WWW (move door left)
                    else if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
                        get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
                        isWall(downLeft) && isWall(down) && isWall(downRight))
                    {
                        // Move door LEFT  
                        Vector2D leftPos = {pos.y, pos.x - 1};
                        set_door(leftPos, leftPos.x, leftPos.y);
                    }
                    else
                    {
                        // All other doors (including WRR/CCR/WRR pattern)
                        set_door(pos, pos.x, pos.y);
                    }
                }
            }
        }
    }
}

void Map::spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx)
{
    itemFactory->spawn_all_enhanced_items_debug(position, ctx);
}

// end of file: Map.cpp
