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
				//game.d.roll(ROOM_MIN_SIZE, node->h - 2), //random int from min size to height - 2
				//game.d.roll(ROOM_MIN_SIZE, node->w - 2) //random int from min size to width - 2
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->h - 2), // random int from min size to height - 2
				map.rng_unique->getInt(ROOM_MIN_SIZE, node->w - 2) // random int from min size to width - 2
			};
			Vector2D begin
			{
				//game.d.roll(node->y + 1, node->y + node->h - end.y - 1), //from node y + 1 to node x + node height - width - 1
				//game.d.roll(node->x + 1, node->x + node->w - end.x - 1) //from node x + 1 to node x + node width - width - 1
				map.rng_unique->getInt(node->y + 1, node->y + node->h - end.y - 1), // from node y + 1 to node x + node height - width - 1
				map.rng_unique->getInt(node->x + 1, node->x + node->w - end.x - 1) // from node x + 1 to node x + node width - width - 1
			};

			// first create a room
			map.create_room(roomNum == 0, begin.x, begin.y, begin.x + end.x - 1 - 1, begin.y + end.y - 1 - 1, withActors);

			if (roomNum != 0)
			{
				if (game.d.d2() == 1) // 50% chance
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
	case TileType::DOOR:
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
	case TileType::DOOR:
		return false;
	case TileType::CORRIDOR:
		return false;
	default:
		return true;
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

void Map::add_item(Vector2D pos)
{
	// 75% chance to spawn an item at all
	if (rng_unique->getInt(1, 100) > 75) return;

	const int dice = rng_unique->getInt(1, 100);

	if (dice < 40) {
		game.create_item<GoldPile>(pos);
		return;
	}
	if (dice < 50) {
		add_weapons(pos);
		return;
	}
	if (dice < 60) {
		game.create_item<HealthPotion>(pos);
		return;
	}
	if (dice < 80) {
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
					set_tile(thisTile, TileType::DOOR, 2);
					tcodMap->setProperties(tileX, tileY, false, false);
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
					set_tile(lastTile, TileType::DOOR, 2); // set the last tile as a door
					tcodMap->setProperties(lastTile.x, lastTile.y, false, false);
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
					set_tile(thisTile, TileType::DOOR, 2); // set the last tile as a door
					tcodMap->setProperties(tileX, tileY, false, false);
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

	return true;
}

void Map::add_monster(Vector2D pos)
{
	static bool dragonPlaced = false; // Flag to track if a dragon has been placed
	const bool placeDragon = !dragonPlaced && game.d.d100() < 5; // 5% chance for a dragon

	if (placeDragon)
	{
		game.create_creature<Dragon>(pos);
		dragonPlaced = true; // Only one dragon per game
	}
	else
	{
		// Determine the monster to spawn based on dungeonLevel
		int roll = game.d.d100(); // Random roll between 1-100

		if (game.dungeonLevel <= 3)
		{
			// Early levels: Mostly Goblins, rare chance for Shopkeeper
			if (roll < 70) game.create_creature<Goblin>(pos); // 70% chance
			else if (roll < 90) game.create_creature<Orc>(pos); // 20% chance
			else if (roll < 98) game.create_creature<Shopkeeper>(pos); // 8% chance
			else game.create_creature<Troll>(pos); // 2% chance
		}
		else if (game.dungeonLevel <= 6)
		{
			// Mid levels: Mix of monsters, higher chance for Shopkeeper
			if (roll < 40) game.create_creature<Goblin>(pos); // 40% chance
			else if (roll < 75) game.create_creature<Orc>(pos); // 35% chance
			else if (roll < 90) game.create_creature<Shopkeeper>(pos); // 15% chance
			else game.create_creature<Troll>(pos); // 10% chance
		}
		else
		{
			// Deep levels: More dangerous creatures, Shopkeeper appears less
			if (roll < 30) game.create_creature<Goblin>(pos); // 30% chance
			else if (roll < 60) game.create_creature<Orc>(pos); // 30% chance
			else if (roll < 85) game.create_creature<Troll>(pos); // 25% chance
			else game.create_creature<Shopkeeper>(pos); // 15% chance
		}
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

// end of file: Map.cpp
