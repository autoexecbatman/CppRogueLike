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
			const bool withActors = gsl::narrow_cast<bool>(userData);

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
	seed(game.d.roll(0, INT_MAX))
{}

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
	refresh(); // Refresh once after all tiles have been drawn
	game.log("Map::render() end");
}
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

	const int dice = rng_unique->getInt(1, 100);

	if (dice < 30)
	{  // Gold pile
		// Create a gold pile with randomized amount based on dungeon level
		int goldAmount = rng_unique->getInt(5, 10 + game.dungeonLevel * 5);
		auto goldPile = std::make_unique<Item>(pos, ActorData{ '$', "gold pile", GOLD_PAIR });
		goldPile->pickable = std::make_unique<Gold>(goldAmount);
		game.container->add(std::move(goldPile));
		return;
	}
	if (dice < 40) {  // Reduced from 50% to 40%
		add_weapons(pos);
		return;
	}
	if (dice < 50) {  // Reduced from 60% to 50%
		game.create_item<HealthPotion>(pos);
		return;
	}
	if (dice < 60) {  // New: 10% chance for food
		// Randomly pick a food type
		std::vector<std::function<void(Vector2D)>> foods = {
			[&](Vector2D p) { game.create_item<Ration>(p); },
			[&](Vector2D p) { game.create_item<Fruit>(p); },
			[&](Vector2D p) { game.create_item<Bread>(p); },
			[&](Vector2D p) { game.create_item<Meat>(p); }
		};

		int randomIndex = rng_unique->getInt(0, foods.size() - 1);
		foods[randomIndex](pos);
		return;
	}
	if (dice < 80) {  // Shifted from 60-80% to 60-80%
		// Randomly pick a scroll from a list
		std::vector<std::function<void(Vector2D)>> scrolls = {
			[&](Vector2D p) { game.create_item<ScrollOfLightningBolt>(p); },
			[&](Vector2D p) { game.create_item<ScrollOfFireball>(p); },
			[&](Vector2D p) { game.create_item<ScrollOfConfusion>(p); }
		};

		int randomIndex = rng_unique->getInt(0, scrolls.size() - 1);
		scrolls[randomIndex](pos);
		return;
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
	if (begin.x > end.x) { std::swap(begin.x, end.x); }
	if (begin.y > end.y) { std::swap(begin.y, end.y); }

	bool isDoorSet = false;
	bool secondDoorSet = false;
	bool thirdDoorSet = false;
	Vector2D lastTile{ 0,0 };
	for (int tileY = begin.y; tileY <= end.y; tileY++)
	{
		for (int tileX = begin.x; tileX <= end.x; tileX++)
		{
			Vector2D thisTile{ tileY, tileX };
			if (!isDoorSet)
			{
				if (is_wall(thisTile))
				{
					set_door(thisTile, tileX, tileY);
					isDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR, 1);
					tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
				}
			}
			else if (!secondDoorSet && isDoorSet) // if the first door is set and the second door is not set
			{
				if (get_tile_type(thisTile) == TileType::FLOOR || get_tile_type(thisTile) == TileType::WATER)
				{
					// set the last tile as a door
					set_door(lastTile, lastTile.x, lastTile.y);
					secondDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR, 1);
					tcodMap->setProperties(tileX,tileY,true,true); // walkable and transparent
				}
			}
			else if (!thirdDoorSet && secondDoorSet && isDoorSet)
			{
				if (get_tile_type(thisTile) == TileType::WALL)
				{
					// set the last tile as a door
					set_door(thisTile, tileX, tileY);
					thirdDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR, 1);
					tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
				}
			}
			else
			{
				set_tile(thisTile, TileType::CORRIDOR, 1);
				tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
			}

			lastTile = Vector2D{ tileY, tileX };
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
	static bool dragonPlaced = false;
	const bool placeDragon = !dragonPlaced && game.d.d100() < 5;

	if (placeDragon)
	{
		game.create_creature<Dragon>(pos);
		dragonPlaced = true;
	}
	else
	{
		int roll = game.d.d100();

		if (game.dungeonLevel <= 3)
		{
			game.create_creature<Mimic>(pos);
			// Early levels: Mostly Goblins, occasional ranged monsters
			if (roll < 65) game.create_creature<Goblin>(pos);      // 65% chance
			else if (roll < 80) game.create_creature<Orc>(pos);    // 15% chance
			else if (roll < 90) game.create_creature<Archer>(pos); // 10% chance
			else if (roll < 95) game.create_creature<Mage>(pos);   // 5% chance
			else if (roll < 98) game.create_creature<Shopkeeper>(pos); // 3% chance
			else game.create_creature<Troll>(pos);                 // 2% chance
		}
		else if (game.dungeonLevel <= 6)
		{
			// Mid levels: More varied enemies
			if (roll < 30) game.create_creature<Goblin>(pos);      // 30% chance
			else if (roll < 55) game.create_creature<Orc>(pos);    // 25% chance
			else if (roll < 70) game.create_creature<Archer>(pos); // 15% chance
			else if (roll < 85) game.create_creature<Mage>(pos);   // 15% chance
			else if (roll < 95) game.create_creature<Shopkeeper>(pos); // 10% chance
			else game.create_creature<Troll>(pos);                 // 5% chance
		}
		else
		{
			// Deep levels: More dangerous creatures
			if (roll < 20) game.create_creature<Goblin>(pos);      // 20% chance
			else if (roll < 40) game.create_creature<Orc>(pos);    // 20% chance
			else if (roll < 55) game.create_creature<Archer>(pos); // 15% chance
			else if (roll < 75) game.create_creature<Mage>(pos);   // 20% chance
			else if (roll < 90) game.create_creature<Troll>(pos);  // 15% chance
			else game.create_creature<Shopkeeper>(pos);            // 10% chance
		}
	}

	// After creating any monster, update its AI if it's ranged
	Creature* monster = get_actor(pos);
	if (monster && monster->has_state(ActorState::IS_RANGED)) {
		// Replace standard AI with ranged AI
		monster->ai = std::make_unique<AiMonsterRanged>();
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
	// clear the actors container except the player and the stairs
	game.creatures.clear();
	game.container->inv.clear();
	game.rooms.clear(); // we clear the room coordinates

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
	game.message(WHITE_PAIR, "You open the door.", true);

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

// end of file: Map.cpp
