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

// a binary space partition listener class (BSP)
class BspListener : public ITCODBspCallback
{
private:
	Map& map; // a map to dig
	int roomNum{ 0 }; // room number
	int lastx{ 0 }, lasty{ 0 }; // center of the last room
public:
	BspListener(Map& map) noexcept : map(map), roomNum(0) {}

	bool visitNode(TCODBsp* node, void* userData) override
	{
		if (node->isLeaf())
		{
			int room_pos_x, room_pos_y, room_width, room_height = 0;
			const bool withActors = gsl::narrow_cast<bool>(userData);
			room_width = map.rng_unique->getInt(ROOM_MIN_SIZE, node->w - 2);//random int from min size to width - 2
			room_height = map.rng_unique->getInt(ROOM_MIN_SIZE, node->h - 2);//random int from min size to height - 2
			room_pos_x = map.rng_unique->getInt(node->x + 1, node->x + node->w - room_width - 1);//from node x + 1 to node x + node width - width - 1
			room_pos_y = map.rng_unique->getInt(node->y + 1, node->y + node->h - room_height - 1);//from node y + 1 to node x + node height - width - 1

			// first create a room
			map.create_room
			(
				roomNum == 0,
				room_pos_x,
				room_pos_y,
				room_pos_x + room_width - 1 - 1,
				room_pos_y + room_height - 1 - 1,
				withActors
			);

			std::cout << "Room " << roomNum << " : " << room_pos_x << ", " << room_pos_y << ", " << room_width << ", " << room_height << std::endl;
			std::clog << "Room " << roomNum << " : " << room_pos_x << ", " << room_pos_y << ", " << room_width << ", " << room_height << std::endl;

			if (roomNum != 0)
			{
				RandomDice d;
				const int rolld2 = d.d2();
				if (rolld2 == 1)
				{
					// dig a corridor from last room
					// place doors
					map.dig(
						room_pos_x + room_width / 2,
						lasty,
						room_pos_x + room_width / 2,
						room_pos_y + room_height / 2
					);
					map.dig(
						lastx,
						lasty,
						room_pos_x + room_width / 2,
						lasty
					);
				/*map.dig(1,10,117,10);*/ // test dig
				}
				else
				{

					// fixed corridor
					map.dig(
						lastx,
						lasty,
						room_pos_x + room_width / 2,
						lasty
					);

					map.dig(
						room_pos_x + room_width / 2,
						lasty,
						room_pos_x + room_width / 2,
						room_pos_y + room_height / 2
					);
					
				}
			}

			lastx = room_pos_x + room_width / 2; // set lastx to center of room
			lasty = room_pos_y + room_height / 2; // set lasty to center of room
			
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
	if (!tiles.empty())
	{
		tiles.clear();
	}
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
		game.player->destructible->hp -= 1;
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
				mvaddch(tile.position.y, tile.position.x, '+');
				attroff(COLOR_PAIR(DOOR_PAIR));
				break;
			default:
				break;
			}
		}
		else
		{
			/*game.log("Map::render() not in FOV");*/
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
	auto weaponActor = std::make_unique<Item>(pos, ActorData{ '/', selectedWeapon.name, rollColor }, ActorFlags{ false, false, false });

	if (selectedWeapon.name == "Dagger")
	{
		weaponActor->pickable = std::make_unique<Dagger>(1, 4);
		game.err("Dagger added");
	}
	else if (selectedWeapon.name == "Long Sword")
	{
		weaponActor->pickable = std::make_unique<LongSword>(1, 8);
		game.err("Long Sword added");
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

		// add gold
		auto gold = std::make_unique<Item>(pos, ActorData{ '$', "gold", GOLD_PAIR }, ActorFlags{ false, false, false });
		gold->flags.blocks = false;
		gold->pickable = std::make_unique<Gold>(game.d.roll(1, 10));
		/*game.container->inv.insert(game.container->inv.begin(), std::move(gold));*/

	}
	else if (dice < 70)
	{
		// add a health potion
		auto healthPotion = std::make_unique<Item>(pos, ActorData{ '!', "health potion", HPBARMISSING_PAIR }, ActorFlags{ false, false, false });
		healthPotion->flags.blocks = false;
		healthPotion->pickable = std::make_unique<Healer>(4);
		game.container->inv.insert(game.container->inv.begin(), std::move(healthPotion));
	}
	else if (dice < 70+10)
	{
		// add lightning scrolls
		auto lightningScroll = std::make_unique<Item>(pos, ActorData{ '#', "scroll of lightning bolt", LIGHTNING_PAIR }, ActorFlags{ false, false, false });
		lightningScroll->flags.blocks = false;
		lightningScroll->pickable = std::make_unique<LightningBolt>(5, 20);
		game.container->inv.insert(game.container->inv.begin(), std::move(lightningScroll));
	}
	else if (dice < 70 + 10 + 10)
	{
		// add fireball scrolls
		auto fireballScroll = std::make_unique<Item>(pos, ActorData{ '#', "scroll of fireball", FIREBALL_PAIR }, ActorFlags{ false, false, false });
		fireballScroll->flags.blocks = false;
		fireballScroll->pickable = std::make_unique<Fireball>(3, 12);
		game.container->inv.insert(game.container->inv.begin(), std::move(fireballScroll));
	}
	else
	{
		// add confusion scrolls
		auto confusionScroll = std::make_unique<Item>(pos, ActorData{ '#', "scroll of confusion", CONFUSION_PAIR }, ActorFlags{ false, false, false });
		confusionScroll->pickable = std::make_unique<Confuser>(10, 8);
		game.container->inv.insert(game.container->inv.begin(), std::move(confusionScroll));
	}
}

void Map::dig(int x1, int y1, int x2, int y2)
{
	if (x2 < x1)
	{
		std::swap(x1, x2);
	}

	if (y2 < y1)
	{
		std::swap(y1, y2);
	}

	RandomDice d;
	// roll d2
	const int rollD2 = d.d2();

	if (rollD2 == 1)
	{
		for (int tileY = y1; tileY <= y2; tileY++)
		{
			for (int tileX = x1; tileX <= x2; tileX++)
			{
				set_tile(Vector2D{ tileY, tileX }, TileType::DOOR);
				tcodMap->setProperties(
					tileX,
					tileY,
					true, // isTransparent
					true // isWalkable
				);
			}
		}
	}
	else
	{
		int width = x2 - x1 + 1;
		int height = y2 - y1 + 1;
		int centerX = (x1 + x2) / 2;
		int centerY = (y1 + y2) / 2;

		for (int tileY = y1; tileY <= y2; tileY++) {
			// Calculate the horizontal range to create a slightly diamond shape
			int halfWidth = (width / 2) * (1 - abs(tileY - centerY) / (float)centerY);
			int startX = centerX - halfWidth;
			int endX = centerX + halfWidth;

			for (int tileX = startX; tileX <= endX; tileX++) {
				if (tileX >= x1 && tileX <= x2) {
					set_tile(Vector2D{ tileY, tileX }, TileType::FLOOR);
					tcodMap->setProperties(
						tileX,
						tileY,
						true, // isTransparent
						true  // isWalkable
					);
				}
			}
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
	dig(x1, y1, x2, y2); // dig the corridors

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
		if (actor->flags.blocks && actor->position == pos)
		{
			return false;
		}
	}

	return true;
}

void Map::add_monster(Vector2D pos)
{
	game.err("player level: " + std::to_string(game.player->playerLevel));
	static bool dragonPlaced = false; // flag to track if a dragon has been placed
	RandomDice d;

	// Determine if this room should contain the dragon
	const bool placeDragon = !dragonPlaced && d.d100() < 5; // 5% chance to place a dragon

	/*auto shopkeeper = std::make_unique<Actor>(mon_y, mon_x, 'S', "shopkeeper", WHITE_PAIR, 0);*/
	auto shopkeeper = std::make_unique<Creature>(pos, ActorData{'S', "shopkeeper", WHITE_PAIR}, ActorFlags{true, false, false });
	shopkeeper->destructible = std::make_unique<MonsterDestructible>(10, 0, "dead shopkeeper", 10, 10, 10);

	shopkeeper->attacker = std::make_unique<Attacker>(3, 1, 10);
	shopkeeper->ai = std::make_unique<AiShopkeeper>();
	shopkeeper->container = std::make_unique<Container>(10);
	// populate the shopkeeper's inventory
	/*auto healthPotion = std::make_unique<Actor>(0, 0, '!', "health potion", HPBARMISSING_PAIR, 0);*/
	auto healthPotion = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '!', "health potion", HPBARMISSING_PAIR }, ActorFlags{ false, false, false });
	/*healthPotion->flags.blocks = false;*/
	healthPotion->pickable = std::make_unique<Healer>(4);
	shopkeeper->container->add(std::move(healthPotion));
	/*auto dagger = std::make_unique<Actor>(0, 0, '/', "dagger", 1, 0);*/
	auto dagger = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '/', "dagger", 1 }, ActorFlags{ false, false, false });
	/*dagger->flags.blocks = false;*/
	dagger->pickable = std::make_unique<Dagger>(1, 4);
	shopkeeper->container->add(std::move(dagger));
	game.creatures.push_back(std::move(shopkeeper));
	game.shopkeeper = game.creatures.back().get();

	if (placeDragon)
	{
		/*auto dragon = std::make_unique<Dragon>(mon_y, mon_x);*/
		auto dragon = std::make_unique<Dragon>(pos);
		game.creatures.push_back(std::move(dragon));
		dragonPlaced = true; // set the flag to true so no more dragons are placed
	}
	else
	{
		const auto roll4d6 = d.d6() + d.d6() + d.d6() + d.d6(); // roll 4d6
		for (auto i{ 0 }; i < roll4d6; i++)
		{ // create goblins
			auto goblin = create_monster<Goblin>(pos);
			game.creatures.push_back(std::move(goblin));
		}

		if (game.player->playerLevel > 3)
		{
			const auto roll2d6 = d.d6() + d.d6(); // roll 2d6
			for (auto i{ 0 }; i < roll2d6; i++)
			{ // create orcs
				auto orc = create_monster<Orc>(pos);
				game.creatures.push_back(std::move(orc));
			}
		}

		if (game.player->playerLevel > 5)
		{
			const auto roll1d6 = d.d6();
			for (auto i{ 0 }; i < roll1d6; i++)
			{ // create trolls
				auto troll = create_monster<Troll>(pos);
				game.creatures.push_back(std::move(troll));
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
