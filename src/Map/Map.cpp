// file: Map.cpp
#include <iostream>
#include <random>
#include <algorithm>
#include <span>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "Map.h"
#include "../Game.h"
#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/Confuser.h"
#include "../Actor/Container.h"
#include "../Actor/Destructible.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Gold.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Player.h"
#include "../Ai/Ai.h"
#include "../Ai/AiShopkeeper.h"
#include "../Colors/Colors.h"
#include "../Random/RandomDice.h"
#include "../Weapons.h"
#include "../Items.h"
#include "../AiMonsterRanged.h"
#include "../Food.h"
#include "../Spider.h"
#include "../MonsterFactory.h"
#include "../ItemFactory.h"

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
	BspListener(Map& map) noexcept : map(map), roomNum(0) {}

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
				withActors
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
	seed(game.d.roll(0, INT_MAX)),
	monsterFactory(std::make_unique<MonsterFactory>()),
	itemFactory(std::make_unique<ItemFactory>())
{
}

void Map::init_tiles()
{
	if (!tiles.empty()) { tiles.clear(); }

	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			tiles.emplace_back(Tile(Vector2D{ y, x }, TileType::WALL, 0));
		}
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors)
{
	init_tiles();
	seed = game.d.roll(0, INT_MAX); // for new seed
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	tcodMap = std::make_unique<TCODMap>(map_width, map_height);
	bsp(map_width, map_height, *rng_unique, withActors);
	tcodPath = std::make_unique<TCODPath>(tcodMap.get(), 1.41f);

	post_process_doors();

	if (game.dungeonLevel > 1) {
		maybe_create_treasure_room(game.dungeonLevel);
	}
	place_amulet();
}

void Map::bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors)
{
	float randomRatio = game.d.d100();
	TCODBsp myBSP(0, 0, map_width, map_height);
	myBSP.splitRecursive(&rng_unique, 4, ROOM_HORIZONTAL_MAX_SIZE, ROOM_VERTICAL_MAX_SIZE, randomRatio, randomRatio);

	BspListener mylistener(*this);
	myBSP.traverseInvertedLevelOrder(&mylistener, (void*)withActors);
	place_stairs();
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
		int cost = tileJson.at("cost").get<double>();

		tiles.emplace_back(position, type, cost);
		tiles.back().explored = explored;
	}

	tcodMap = std::make_unique<TCODMap>(map_width, map_height);
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	bsp(map_width, map_height, *rng_unique, false);
	tcodPath = std::make_unique<TCODPath>(tcodMap.get(), 1.41f);

	// New: Post-process to place doors after loading map
	post_process_doors();
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

bool Map::is_wall(Vector2D pos) const
{
	return !tcodMap->isWalkable(pos.x, pos.y);
}

bool Map::is_explored(Vector2D pos) const
{
	return tiles.at(get_index(pos)).explored;
}

void Map::set_explored(Vector2D pos)
{
	tiles.at(get_index(pos)).explored = true;
}

bool Map::is_in_fov(Vector2D pos) const
{
	return tcodMap->isInFov(pos.x, pos.y);
}

bool Map::is_water(Vector2D pos) const
{
	return tiles.at(get_index(pos)).type == TileType::WATER;
}

TileType Map::get_tile_type(Vector2D pos) const
{
	return tiles.at(get_index(pos)).type;
}

// returns true if the player after a successful move
void Map::tile_action(Creature& owner, TileType tileType)
{
	switch (tileType)
	{
	case TileType::WATER:
		game.log("You are in water");
		game.message(COLOR_WHITE, "You are in water", true);
		owner.destructible->take_damage(owner, 1);
		break;
	case TileType::WALL:
		game.log("You are against a wall");
		game.message(COLOR_WHITE, "You are against a wall", true);
		break;
	case TileType::FLOOR:
		game.log("You are on the floor");
		game.message(COLOR_WHITE, "You are on the floor", true);
		break;
	case TileType::CLOSED_DOOR:
		game.log("You are at a door");
		game.message(COLOR_WHITE, "You are at a door", true);
		break;
	case TileType::CORRIDOR:
		game.log("You are in a corridor");
		game.message(COLOR_WHITE, "You are in a corridor", true);
		break;
	default:
		game.log("You are in an unknown area");
	}
}

bool Map::is_collision(Creature& owner, TileType tileType, Vector2D pos)
{
	// if there is an actor at the position
	const auto& actor = get_actor(pos);
	if (actor)
	{
		return true;
	}

	switch (tileType)
	{
	case TileType::WATER:
		return owner.has_state(ActorState::CAN_SWIM) ? false : true;
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

void Map::compute_fov()
{
	game.log("...Computing FOV...");
	// Safety check for valid player position
	if (!game.player ||
		game.player->position.x < 0 || game.player->position.x >= map_width ||
		game.player->position.y < 0 || game.player->position.y >= map_height) {
		game.log("Warning: Can't compute FOV - invalid player position");
		return;
	}
	tcodMap->computeFov(game.player->position.x, game.player->position.y, FOV_RADIUS, true, FOV_SYMMETRIC_SHADOWCAST);
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
	game.log("Map::render()");
	for (const auto& tile : tiles)
	{
		if (is_in_fov(tile.position) || is_explored(tile.position))
		{
			switch (get_tile_type(tile.position))
			{
			case TileType::WALL:
				attron(COLOR_PAIR(WALL_PAIR));
				mvaddch(tile.position.y, tile.position.x, '#');
				attroff(COLOR_PAIR(WALL_PAIR));
				break;
			case TileType::WATER:
				attron(COLOR_PAIR(WATER_PAIR));
				mvaddch(tile.position.y, tile.position.x, '~');
				attroff(COLOR_PAIR(WATER_PAIR));
				break;
			case TileType::FLOOR:
				mvaddch(tile.position.y, tile.position.x, '.');
				break;
			case TileType::CLOSED_DOOR:
				attron(COLOR_PAIR(DOOR_PAIR));
				mvaddch(tile.position.y, tile.position.x, '+'); // Closed door character
				attroff(COLOR_PAIR(DOOR_PAIR));
				break;
			case TileType::OPEN_DOOR:
				attron(COLOR_PAIR(DOOR_PAIR));
				mvaddch(tile.position.y, tile.position.x, '/'); // Different symbol for open door
				attroff(COLOR_PAIR(DOOR_PAIR));
				break;
			case TileType::CORRIDOR:
				//attron(COLOR_PAIR(HPBARFULL_PAIR));
				mvaddch(tile.position.y, tile.position.x, '.');
				//attroff(COLOR_PAIR(HPBARFULL_PAIR));
				break;
			default:
				break;
			}
		}
	}
	game.log("Map::render() end");
}

// this function is deprecated
void Map::add_weapons(Vector2D pos)
{
	const int weaponIndex{ game.d.roll(1, game.weapons.size()) };
	const Weapons& selectedWeapon{ game.weapons.at(weaponIndex - 1) };
	const int rollColor{ game.d.roll(1, game.weapons.size()) };

	// Create an Actor for the weapon
	auto weaponItem = std::make_unique<Item>(pos, ActorData{ '/', selectedWeapon.name, rollColor });

	// Map weapon names to corresponding constructors
	static const std::unordered_map<std::string, std::function<std::unique_ptr<Pickable>()>> weaponFactory{
		{"Dagger", []() { return std::make_unique<Dagger>(); }},
		{"Long Sword", []() { return std::make_unique<LongSword>(); }},
		{"Short Sword", []() { return std::make_unique<ShortSword>(); }},
		{"Longbow", []() { return std::make_unique<Longbow>(); }},
		{"Staff", []() { return std::make_unique<Staff>(); }}
	};

	auto it = weaponFactory.find(selectedWeapon.name);
	if (it != weaponFactory.end())
	{
		weaponItem->pickable = it->second();
	}
	else
	{
		game.log("Error: Unknown weapon type");
		return;
	}

	// Add the weapon to the game container
	game.container->add(std::move(weaponItem));
}

void Map::add_item(Vector2D pos) {
	// 75% chance to spawn an item at all
	if (rng_unique->getInt(1, 100) > 75) return;

	// Use our ItemFactory to create a random item
	itemFactory->spawn_random_item(pos, game.dungeonLevel);
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
		int height = end.y - begin.y + 1;
		int centerX = (begin.x + end.x) / 2;
		int centerY = (begin.y + end.y) / 2;

		for (int tileY = begin.y; tileY <= end.y; tileY++) {
			// Calculate the horizontal range to create a slightly diamond shape
			int halfWidth = (width / 2) * (1 - abs(tileY - centerY) / (float)centerY);
			int startX = centerX - halfWidth;
			int endX = centerX + halfWidth;

			for (int tileX = startX; tileX <= endX; tileX++) {
				if (tileX >= begin.x && tileX <= end.x) {
					set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR, 1);
					tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
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
    bool horizontal_first = game.d.roll(0,1);

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
	tiles.at(get_index(pos)).type = newType;
	tiles.at(get_index(pos)).cost = cost;
}

void Map::create_room(bool first, int x1, int y1, int x2, int y2, bool withActors)
{
	Vector2D begin{ y1,x1 };
	Vector2D end{ y2,x2 };

	// store the room coordinates
	game.rooms.emplace_back(begin);
	game.rooms.emplace_back(end);

	dig(begin, end); // dig the corridors

	spawn_water(begin, end);

	if (!withActors)
	{
		return;
	}

	if (first) // if this is the first room, we need to place the player in it
	{
		spawn_player(begin, end);
	}

	spawn_items(begin, end);
}

void Map::spawn_water(Vector2D begin, Vector2D end)
{
	// Add water tiles
	constexpr int waterPercentage = 10; // 10% of tiles will be water, adjust as needed
	Vector2D waterPos{ 0,0 };
	for (waterPos.y = begin.y; waterPos.y <= end.y; ++waterPos.y)
	{
		for (waterPos.x = begin.x; waterPos.x <= end.x; ++waterPos.x)
		{
			/*const int rolld100 = game.d.d100();*/
			const int rolld100 = rng_unique->getInt(1, 100);
			if (rolld100 < waterPercentage)
			{
				set_tile(waterPos, TileType::WATER, 10);
				tcodMap->setProperties(waterPos.x, waterPos.y, true, true); // non-walkable and non-transparent
			}
		}
	}
}

void Map::spawn_items(Vector2D begin, Vector2D end)
{
	// add items
	const int numItems = game.d.roll(0, MAX_ROOM_ITEMS);
	for (int i = 0; i < numItems; i++)
	{
		Vector2D itemPos{ game.d.roll(begin.y, end.y),game.d.roll(begin.x, end.x) };
		while (!can_walk(itemPos) || is_stairs(itemPos))
		{
			itemPos.x = game.d.roll(begin.x, end.x);
			itemPos.y = game.d.roll(begin.y, end.y);
		}
		add_item(itemPos);
	}
}

void Map::spawn_player(Vector2D begin, Vector2D end)
{
	// add player
	Vector2D playerPos{ game.d.roll(begin.y, end.y),game.d.roll(begin.x, end.x) };
	while (!can_walk(playerPos))
	{
		playerPos.x = game.d.roll(begin.x, end.x);
		playerPos.y = game.d.roll(begin.y, end.y);
	}
	game.player->position.x = playerPos.x;
	game.player->position.y = playerPos.y;
}

void Map::place_stairs() const
{
	int index = game.d.roll(0, static_cast<int>(game.rooms.size()) - 1);
	index = index % 2 == 0 ? index : index - 1;
	const Vector2D roomBegin = game.rooms.at(index);
	const Vector2D roomEnd = game.rooms.at(index + 1);
	Vector2D stairsPos{ game.d.roll(roomBegin.y, roomEnd.y),game.d.roll(roomBegin.x, roomEnd.x) };
	while (!can_walk(stairsPos))
	{
		stairsPos.x = game.d.roll(roomBegin.x, roomEnd.x);
		stairsPos.y = game.d.roll(roomBegin.y, roomEnd.y);
	}
	game.stairs->position.x = stairsPos.x;
	game.stairs->position.y = stairsPos.y;
}

bool Map::is_stairs(Vector2D pos) const
{
	return game.stairs->position == pos;
}

bool Map::can_walk(Vector2D pos) const
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

	return true;
}

void Map::add_monster(Vector2D pos)
{
	// Use the monster factory to create a monster appropriate for the current dungeon level
	monsterFactory->spawnRandomMonster(pos, game.dungeonLevel);

	// Log the spawn for debugging
	Creature* monster = get_actor(pos);
	if (monster) {
		game.log("Spawned " + monster->actorData.name + " at level " + std::to_string(game.dungeonLevel));
	}
}

// getActor returns the actor at the given coordinates or `nullptr` if there's none
Creature* Map::get_actor(Vector2D pos) noexcept
{
	for (const auto& actor : game.creatures)
	{
		if (actor->position == pos)
		{
			return actor.get();
		}
	}

	return nullptr;
}

std::vector<std::vector<Tile>> Map::get_map() const
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
void Map::regenerate()
{
	clear();
	refresh();
	// clear the actors container except the player and the stairs
	game.creatures.clear();
	game.container->inv.clear();
	game.rooms.clear(); // we clear the room coordinates
	game.objects.clear();

	// generate a new map
	game.map->map_height = MAP_HEIGHT;
	game.map->map_width = MAP_WIDTH;
	game.map->init(true);
}

std::vector<Vector2D> Map::neighbors(Vector2D id) const
{
	std::vector<Vector2D> results;

	for (Vector2D dir : DIRS)
	{
		Vector2D next{ id.y + dir.y, id.x + dir.x };
		if (in_bounds(next) && can_walk(next))
		{
			results.push_back(next);
		}
	}

	if ((id.x + id.y) % 2 == 0)
	{
		// see "Ugly paths" section for an explanation:
		std::reverse(results.begin(), results.end());
	}

	return results;
}

double Map::cost(Vector2D from_node, Vector2D to_node)
{
	// if there is an actor on the tile, return a high cost
	if (get_actor(to_node) != nullptr)
	{
		return 1000.0;
	}
	return get_cost(to_node);
}

bool Map::has_los(Vector2D from, Vector2D to) const
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

bool Map::is_door(Vector2D pos) const
{
	if (pos.y < 0 || pos.y >= map_height || pos.x < 0 || pos.x >= map_width)
		return false;

	TileType tileType = get_tile_type(pos);
	return tileType == TileType::CLOSED_DOOR || tileType == TileType::OPEN_DOOR;
}

bool Map::is_open_door(Vector2D pos) const
{
	if (pos.y < 0 || pos.y >= map_height || pos.x < 0 || pos.x >= map_width)
		return false;
	return get_tile_type(pos) == TileType::OPEN_DOOR;
}

bool Map::open_door(Vector2D pos)
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

	if (game.player && game.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov();
	}

	return true;
}

bool Map::close_door(Vector2D pos)
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
	if (get_actor(pos) != nullptr)
	{
		return false;
	}

	// Change the tile type to DOOR (closed)
	set_tile(pos, TileType::CLOSED_DOOR, 2);

	// Make the tile non-walkable and non-transparent
	tcodMap->setProperties(pos.x, pos.y, false, false);

	if (game.player && game.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov();
	}

	return true;
}

void Map::place_amulet() const
{
	// Only place the amulet on the final level
	if (game.dungeonLevel == FINAL_DUNGEON_LEVEL)
	{
		// Choose a random room for the amulet
		int index = game.d.roll(0, static_cast<int>(game.rooms.size()) - 1);
		index = index % 2 == 0 ? index : index - 1;
		const Vector2D roomBegin = game.rooms.at(index);
		const Vector2D roomEnd = game.rooms.at(index + 1);

		// Find a walkable position in the room
		Vector2D amuletPos{ game.d.roll(roomBegin.y, roomEnd.y), game.d.roll(roomBegin.x, roomEnd.x) };
		while (!can_walk(amuletPos) || is_stairs(amuletPos))
		{
			amuletPos.x = game.d.roll(roomBegin.x, roomEnd.x);
			amuletPos.y = game.d.roll(roomBegin.y, roomEnd.y);
		}

		// Create and place the amulet
		game.create_item<AmuletOfYendor>(amuletPos);

		// Log the placement (debug info)
		game.log("Placed Amulet of Yendor at " + std::to_string(amuletPos.x) + "," + std::to_string(amuletPos.y));

		// Add a hint message
		game.message(FIREBALL_PAIR, "You sense a powerful artifact somewhere on this level...", true);
	}
}

void Map::display_spawn_rates() const
{
	WINDOW* ratesWindow = newwin(
		24,  // height (adjust to fit all monsters)
		50,  // width
		1,   // y position
		1    // x position
	);

	box(ratesWindow, 0, 0);
	mvwprintw(ratesWindow, 1, 1, "Monster Spawn Rates (Dungeon Level %d)", game.dungeonLevel);
	mvwprintw(ratesWindow, 2, 1, "--------------------------------------");

	// Get current distribution from monster factory
	auto distribution = monsterFactory->getCurrentDistribution(game.dungeonLevel);

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

void Map::create_treasure_room(Vector2D begin, Vector2D end, int quality)
{
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
			if (game.d.d100() <= 20) {
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
	itemFactory->generate_treasure(center, game.dungeonLevel, quality);

	// Add guardians or traps based on quality
	int guardianCount = 0;
	switch (quality)
	{
	case 1:
	{
		guardianCount = game.d.roll(0, 1);
		break;
	}
	case 2:
	{
		guardianCount = game.d.roll(1, 2);
		break;
	}
	case 3:
	{
		guardianCount = game.d.roll(2, 3);
		break;
	}
	}

	for (int i = 0; i < guardianCount; i++)
	{
		Vector2D guardPos;
		do
		{
			guardPos.y = game.d.roll(begin.y, end.y);
			guardPos.x = game.d.roll(begin.x, end.x);
			// Ensure we don't place on the center (where treasure is)
			// and the position is valid
		} while ((guardPos.y == center.y && guardPos.x == center.x) ||
			!can_walk(guardPos) ||
			get_actor(guardPos) != nullptr);

		// Create a guardian appropriate for the treasure quality
		add_monster(guardPos);
	}

	game.log("Created treasure room at (" + std::to_string(begin.x) + "," +
		std::to_string(begin.y) + ") to (" + std::to_string(end.x) + "," +
		std::to_string(end.y) + ") with quality " + std::to_string(quality));
}

bool Map::maybe_create_treasure_room(int dungeonLevel)
{
	// Probability increases with dungeon level
	int treasureRoomChance = 5 + (dungeonLevel * 2); // 5% + 2% per level

	// Cap at 25%
	treasureRoomChance = std::min(treasureRoomChance, 25);

	if (game.d.d100() > treasureRoomChance)
	{
		return false; // No treasure room this time
	}

	// Find a suitable room from the rooms list
	if (game.rooms.size() < 2)
	{
		return false; // Need at least one room (which is 2 entries in the vector)
	}

	// Select a random room (skipping the first room, which usually has the player)
	int index = game.d.roll(2, static_cast<int>(game.rooms.size() - 1));
	index = index % 2 == 0 ? index : index - 1; // Ensure it's even (room starts)

	if (index >= static_cast<int>(game.rooms.size()))
	{
		return false; // Safety check
	}

	const Vector2D roomBegin = game.rooms.at(index);
	const Vector2D roomEnd = game.rooms.at(index + 1);

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

	int qualityRoll = game.d.d100();
	if (qualityRoll <= 5 + dungeonLevel)
	{
		quality = 3; // Exceptional (5% + dungeonLevel%)
	}
	else if (qualityRoll <= 15 + (dungeonLevel * 2))
	{
		quality = 2; // Good (15% + dungeonLevel*2%)
	}

	// Create the treasure room
	create_treasure_room(roomBegin, roomEnd, quality);

	return true;
}

void Map::display_item_distribution() const
{
	WINDOW* distributionWindow = newwin(
		24,  // height (adjust to fit all items)
		50,  // width
		1,   // y position
		1    // x position
	);

	box(distributionWindow, 0, 0);
	mvwprintw(distributionWindow, 1, 1, "Item Spawn Rates (Dungeon Level %d)", game.dungeonLevel);
	mvwprintw(distributionWindow, 2, 1, "--------------------------------------");

	// Get current distribution from item factory
	std::vector<ItemPercentage> distribution = itemFactory->get_current_distribution(game.dungeonLevel);

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
                    
                    // 90° rotation: WWR/CWR/CWR
                    if (isWall(upLeft) && isWall(up) && isRoom(upRight) &&
                        get_tile_type(left) == TileType::CORRIDOR && isRoom(right) &&
                        get_tile_type(downLeft) == TileType::CORRIDOR && isWall(down) && isRoom(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // 180° rotation: WWW/CCW/RRR
                    if (isWall(upLeft) && isWall(up) && isWall(upRight) &&
                        get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
                        isRoom(downLeft) && isRoom(down) && isRoom(downRight))
                    {
                        shouldExclude = true;
                    }
                    
                    // 270° rotation: RWC/RWC/RWW
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
// end of file: Map.cpp
