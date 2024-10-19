// file: Map.cpp
#include <iostream>
#include <random>
#include <algorithm>
#include <gsl/util>
#include <gsl/pointers>
#include <gsl/span>
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
#include "../ActorTypes/Dagger.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Gold.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/LongSword.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Player.h"
#include "../Ai/Ai.h"
#include "../Ai/AiShopkeeper.h"
#include "../Colors/Colors.h"
#include "../Random/RandomDice.h"
#include "../Weapons.h"
#include "../Items.h"

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
	Vector2D last{ 0,0 }; // center of the last room
public:
	BspListener(Map& map) noexcept : map(map), roomNum(0) {}

	bool visitNode(TCODBsp* node, void* userData) override
	{
		if (node->isLeaf())
		{
			const bool withActors = gsl::narrow_cast<bool>(userData);
			Vector2D end
			{ 
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->h - 2), //random int from min size to height - 2
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->w - 2) //random int from min size to width - 2
			};
			Vector2D begin
			{
				map.rng_unique->getInt(node->y + 1, node->y + node->h - end.y - 1), //from node y + 1 to node x + node height - width - 1
				map.rng_unique->getInt(node->x + 1, node->x + node->w - end.x - 1) //from node x + 1 to node x + node width - width - 1
			};

			// first create a room
			map.create_room(roomNum == 0, begin.x, begin.y, begin.x + end.x - 1 - 1, begin.y + end.y - 1 - 1, withActors);

			if (roomNum != 0)
			{
				const int rolld2 = game.d.d2();
				if (rolld2 == 1)
				{
					// dig a corridor from last room
					map.dig_corridor({ last.y,begin.x + end.x / 2 }, { begin.y + end.y / 2,begin.x + end.x / 2 });
					map.dig_corridor({ last.y,last.x }, { last.y,begin.x + end.x / 2 });
					return true;
				///*map.dig(1,10,117,10);*/ // test dig
				}
				else
				{
					// fixed corridor
					map.dig_corridor({ last.y,last.x }, { last.y,begin.x + end.x / 2 });
					map.dig_corridor({ last.y,begin.x + end.x / 2 },{ begin.y + end.y / 2,begin.x + end.x / 2 }
					);
					return true;
				}
			}

			last = Vector2D{ begin.y + end.y / 2,begin.x + end.x / 2 };
			
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
	rng_unique(std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC)),
	seed(TCODRandom::getInstance()->getInt(0, INT_MAX))
{}

void Map::init_tiles()
{
	if (!tiles.empty()) { tiles.clear(); }

	for (auto y{ 0 }; y < map_height; y++)
	{
		for (auto x{ 0 }; x < map_width; x++)
		{
			Vector2D pos{ 0, 0 };
			pos.y = y;
			pos.x = x;
			tiles.emplace_back(Tile(pos, TileType::WALL));
		}
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors)
{
	init_tiles();
	seed = TCODRandom::getInstance()->getInt(0, INT_MAX);
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	tcodMap = std::make_unique<TCODMap>(map_width, map_height);

	tcodPath = std::make_unique<TCODPath>(tcodMap.get(), 1.41f);
	
	bsp(map_width, map_height, *rng_unique, withActors);
}

void Map::bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors)
{
	RandomDice d;
	float randomRatio = d.d100();
	TCODBsp myBSP(0, 0, map_width, map_height);
	myBSP.splitRecursive(&rng_unique, 4, ROOM_HORIZONTAL_MAX_SIZE, ROOM_VERTICAL_MAX_SIZE, randomRatio, randomRatio);

	BspListener mylistener(*this);
	myBSP.traverseInvertedLevelOrder(&mylistener, (void*)withActors);
}

void Map::load(TCODZip& zip)
{
	seed = zip.getInt();
	init(false);

	for (auto& tile : tiles)
	{
		tile.explored = gsl::narrow_cast<bool>(zip.getInt());
	}
}

void Map::save(TCODZip& zip)
{
	zip.putInt(seed);

	for (const auto& tile : tiles)
	{
		zip.putInt(gsl::narrow_cast<int>(tile.explored));
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

TileType Map::get_tile_t(Vector2D pos) const
{
	return tiles.at(get_index(pos)).type;
}

bool Map::tile_action(TileType tileType)
{
	switch (tileType)
	{
	case TileType::WATER:
		game.log("You are in water");
		game.message(COLOR_WHITE, "You are in water", true);
		/*game.player->destructible->hp -= 1;*/
		game.player->destructible->take_damage(*game.player, 1);
		return game.player->has_state(ActorState::CAN_SWIM) ? true : false;
	case TileType::WALL:
		game.log("You are against a wall");
		game.message(COLOR_WHITE, "You are against a wall", true);
		return false;
	case TileType::FLOOR:
		game.log("You are on the floor");
		game.message(COLOR_WHITE, "You are on the floor", true);
		return true;
	case TileType::DOOR:
		game.log("You are at a door");
		game.message(COLOR_WHITE, "You are at a door", true);
		return true;
	case TileType::CORRIDOR:
		game.log("You are in a corridor");
		game.message(COLOR_WHITE, "You are in a corridor", true);
		return true;
	default:
		game.log("You are in an unknown area");
		return false;
	}
}

void Map::compute_fov()
{
	game.log("...Computing FOV...");
	tcodMap->computeFov(game.player->position.x, game.player->position.y, FOV_RADIUS);
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
			switch (get_tile_t(tile.position))
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
			case TileType::DOOR:
				attron(COLOR_PAIR(DOOR_PAIR));
				mvprintw(tile.position.y, tile.position.x, "+");
				attroff(COLOR_PAIR(DOOR_PAIR));
				break;
			case TileType::CORRIDOR:
				attron(COLOR_PAIR(WHITE_PAIR));
				mvaddch(tile.position.y, tile.position.x, '.');
				attroff(COLOR_PAIR(WHITE_PAIR));
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
	// Randomly select a weapon from the list
	const int weaponIndex{ game.d.roll(1, game.weapons.size()) };
	const Weapons& selectedWeapon{ game.weapons.at(weaponIndex - 1) };
	const int rollColor{ game.d.roll(1,3) }; // Randomly select a color for the weapon

	// Create an Actor for the weapon
	auto weaponActor = std::make_unique<Item>(pos, ActorData{ '/', selectedWeapon.name, rollColor });

	if (selectedWeapon.name == "Dagger")
	{
		weaponActor->pickable = std::make_unique<Dagger>();
	}
	else if (selectedWeapon.name == "Long Sword")
	{
		weaponActor->pickable = std::make_unique<LongSword>();
	}
	else
	{
		game.log("Error: Unknown weapon type");
		return;
	}

	game.container->add(std::move(weaponActor));
}

void Map::add_item(Vector2D pos)
{
	const int dice{ game.d.d100() };
	if (dice < 60)
	{
		add_weapons(pos);

		game.create_item<GoldPile>(pos);
	}
	else if (dice < 70)
	{
		game.create_item<HealthPotion>(pos);
	}
	else if (dice < 70+10)
	{
		game.create_item<ScrollOfLightningBolt>(pos);
	}
	else if (dice < 70 + 10 + 10)
	{
		game.create_item<ScrollOfFireball>(pos);
	}
	else
	{
		game.create_item<ScrollOfConfusion>(pos);
	}
}

void Map::dig(Vector2D begin, Vector2D end)
{
	if (begin.x > end.x) { std::swap(begin.x, end.x); }
	if (begin.y > end.y) { std::swap(begin.y, end.y); }

	const int rollD2 = game.d.d2(); // 50% to dig square or diamond shape
	if (rollD2 == 1)
	{
		for (int tileY = begin.y; tileY <= end.y; tileY++)
		{
			for (int tileX = begin.x; tileX <= end.x; tileX++)
			{
				set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR);
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
					set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR);
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
					set_tile(thisTile, TileType::DOOR);
					tcodMap->setProperties(tileX, tileY, false, false);
					isDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR);
					tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
				}
			}
			else if (!secondDoorSet && isDoorSet) // if the first door is set and the second door is not set
			{
				if (get_tile_t(thisTile) == TileType::FLOOR || get_tile_t(thisTile) == TileType::WATER)
				{
					set_tile(lastTile, TileType::DOOR); // set the last tile as a door
					tcodMap->setProperties(lastTile.x, lastTile.y, false, false);
					secondDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR);
					tcodMap->setProperties(tileX,tileY,true,true); // walkable and transparent
				}
			}
			else if (!thirdDoorSet && secondDoorSet && isDoorSet)
			{
				if (get_tile_t(thisTile) == TileType::WALL)
				{
					set_tile(thisTile, TileType::DOOR); // set the last tile as a door
					tcodMap->setProperties(tileX, tileY, false, false);
					thirdDoorSet = true;
				}
				else
				{
					set_tile(thisTile, TileType::CORRIDOR);
					tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
				}
			}
			else
			{
				set_tile(thisTile, TileType::CORRIDOR);
				tcodMap->setProperties(tileX, tileY, true, true); // walkable and transparent
			}

			lastTile = Vector2D{ tileY, tileX };
		}
	}
}

void Map::set_tile(Vector2D pos, TileType newType)
{
	tiles.at(get_index(pos)).type = newType;
}

void Map::create_room(bool first, int x1, int y1, int x2, int y2, bool withActors)
{
	RandomDice d;
	Vector2D begin{ y1,x1 };
	Vector2D end{ y2,x2 };
	dig(begin, end); // dig the corridors

	// Add water tiles
	constexpr int waterPercentage = 10; // 10% of tiles will be water, adjust as needed
	Vector2D waterPos{ 0,0 };
	for (waterPos.x = x1; waterPos.x <= x2; ++waterPos.x)
	{
		for (waterPos.y = y1; waterPos.y <= y2; ++waterPos.y)
		{
			const int rolld100 = d.d100();
			if (rolld100 < waterPercentage)
			{
				// Assuming you have a set_tile function that sets the tile at (x, y) to water
				set_tile(waterPos, TileType::WATER);
			}
		}
	}

	if (!withActors)
	{
		return;
	}

	if (first) // if this is the first room, we need to place the player in it
	{
		game.player->position.y = y1 + 3;
		game.player->position.x = x1 + 4;

		// create a player from the player class and place it in the room
	}
	
	// If this is NOT the first room, we make a random number of monsters and place them in the room
	// First we get a random number of monsters and for each one, get a random position inside the room.
	// If the tile is empty (canWalk) we create a monster.
	
	else
	{
		const int numMonsters = TCODRandom::getInstance()->getInt(0, MAX_ROOM_MONSTERS);
		for (int i = 0; i < numMonsters; i++)
		{
			Vector2D monsterPos{ TCODRandom::getInstance()->getInt(y1, y2),TCODRandom::getInstance()->getInt(x1, x2) };

			if (is_wall(monsterPos))
			{
				continue;
			}

			add_monster(monsterPos);
		}

		// add stairs
		game.stairs->position.x = x1 + 4;
		game.stairs->position.y = y1 + 2;
	} 
	
	// add items
	const int numItems = TCODRandom::getInstance()->getInt(0, MAX_ROOM_ITEMS);

	for (int i = 0; i < numItems; i++)
	{
		Vector2D itemPos{ TCODRandom::getInstance()->getInt(y1, y2),TCODRandom::getInstance()->getInt(x1, x2) };

		if (is_wall(itemPos))
		{
			continue;
		}

		add_item(itemPos);
	}

}

//====
// We want to detect, when the player tries to walk on a Tile , if it is occupied by another actor.
bool Map::can_walk(Vector2D pos) const
{
	
	if (is_wall(pos)) // check if the tile is a wall
	{
		return false;
	}
	if (is_water(pos)) // check if the tile is water
	{
		return false;
	}

	for (const auto& actor : game.creatures)
	{
		if (actor->has_state(ActorState::BLOCKS) && actor->position == pos)
		{
			return false;
		}
	}

	return true;
}

void Map::add_monster(Vector2D pos)
{
	auto& d = game.d; // get the random dice
	static bool dragonPlaced = false; // flag to track if a dragon has been placed
	// Determine if this room should contain the dragon
	const bool placeDragon = !dragonPlaced && d.d100() < 5; // 5% chance to place a dragon

	/*game.create_monster<Shopkeeper>(pos);*/

	if (placeDragon)
	{
		game.create_monster<Dragon>(pos);
		dragonPlaced = true; // set the flag to true so no more dragons are placed
	}
	else
	{
		const auto roll4d6 = d.d6() + d.d6() + d.d6() + d.d6(); // roll 4d6
		for (auto i{ 0 }; i < roll4d6; i++)
		{ // create goblins
			game.create_monster<Goblin>(pos);
		}

		game.create_monster<Goblin>(pos);

		if (game.player->playerLevel > 3)
		{
			const auto roll2d6 = d.d6() + d.d6(); // roll 2d6
			for (auto i{ 0 }; i < roll2d6; i++)
			{ // create orcs
				game.create_monster<Orc>(pos);
			}
		}

		if (game.player->playerLevel > 5)
		{
			const auto roll1d6 = d.d6();
			for (auto i{ 0 }; i < roll1d6; i++)
			{ // create trolls
				game.create_monster<Troll>(pos);
			}
		}
	}
}

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

	// generate a new map
	game.map->map_height = MAP_HEIGHT;
	game.map->map_width = MAP_WIDTH;
	game.map->init(true);
}

// end of file: Map.cpp
