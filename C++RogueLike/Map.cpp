// file: Map.cpp
#include <iostream>
#include <random>
#include <gsl/util>
#include <gsl/pointers>
#include <gsl/span>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include "Game.h"
#include "Map.h"
#include "Persistent.h"
#include "Destructible.h"
#include "Dagger.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Healer.h"
#include "LightningBolt.h"
#include "LongSword.h"
#include "Fireball.h"
#include "Confuser.h"
#include "Container.h"
#include "Actor.h"
#include "Colors.h"
#include "Goblin.h"
#include "RandomDice.h"
#include "Player.h"
#include "Weapons.h"
#include "Gold.h"

//#include "/Repositories/CaptainCrowbar/dice/source/dice/dice.hpp"
//#include "/Repositories/CaptainCrowbar/dice/source/dice/rational.hpp"

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
				// dig a corridor from last room
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

			lastx = room_pos_x + room_width / 2; // set lastx to center of room
			lasty = room_pos_y + room_height / 2; // set lasty to center of room
			
			roomNum++;
		}
		return true;
	}
};

void Map::load(TCODZip& zip)
{
	seed = zip.getInt();
	init(false);
	for (int i = 0; i < map_width * map_height; i++)
	{
		const gsl::span<Tile> tiles_span(tiles.get(), map_width * map_height);
		tiles_span[i].explored = gsl::narrow_cast<bool>(zip.getInt());
	}
}

void Map::save(TCODZip& zip)
{
	zip.putInt(seed);

	if (!tiles) { std::cout << "Map::save: tiles is null" << std::endl; exit(-1); }

	for (int i = 0; i < map_width * map_height; i++) 
	{
		const gsl::span<Tile> tiles_span(tiles.get(), map_width * map_height);
		zip.putInt(gsl::narrow_cast<int>(tiles_span[i].explored));
	}
}

void Map::bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors)
{
	TCODBsp myBSP = TCODBsp(0, 0, map_width, map_height);
	myBSP.splitRecursive(&rng_unique, 4, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
	BspListener mylistener = BspListener(*this);
	myBSP.traverseInvertedLevelOrder(&mylistener, (void*)withActors);
}

//====
//In Map.cpp, we allocate the TCODMap object in the constructor
Map::Map(int map_height, int map_width)
	:
	map_height(map_height),
	map_width(map_width),
	tcodMap(nullptr),
	tiles(nullptr)
{
	// a random seed for the map
	seed = TCODRandom::getInstance()->getInt(0, 0x7FFFFFFF); // 0x7FFFFFFF is the highest possible 32 bit signed integer value.
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors)
{
	rng_unique = std::make_unique<TCODRandom>(seed, TCOD_RNG_CMWC);
	tiles = std::make_unique<Tile[]>(map_height * map_width);
	tcodMap = std::make_unique<TCODMap>(map_width, map_height);
	bsp(map_width, map_height, *rng_unique, withActors);
}

bool Map::is_wall(int isWall_pos_y, int isWall_pos_x) const // checks if it is a wall?
{
	// return !tiles[isWall_pos_x + isWall_pos_y * map_width].canWalk;

	return !tcodMap->isWalkable(
		isWall_pos_x,
		isWall_pos_y
	);
}

bool Map::is_explored(int exp_x, int exp_y) const noexcept
{
	return gsl::span(tiles.get(), map_width * map_height)[exp_x + (exp_y * map_width)].explored;

}

bool Map::is_in_fov(int fov_x, int fov_y) const
{
	// Check if the coordinates are out of bounds
	if (fov_x < 0 || fov_x >= MAP_WIDTH || fov_y < 0 || fov_y >= MAP_HEIGHT)
	{
		return false;
	}

	// If tcodMap is null, log an error and return false
	if (tcodMap == nullptr)
	{
		std::clog << "Error: tcodMap is null" << std::endl;
		return false;
	}

	// If the coordinates are in field of view, mark the tile as explored
	if (tcodMap->isInFov(fov_x, fov_y))
	{
		gsl::span(tiles.get(), MAP_WIDTH * MAP_HEIGHT)[fov_x + (fov_y * MAP_WIDTH)].explored = true;
		return true;
	}

	// If the coordinates are not in field of view, return false
	return false;
}

bool Map::is_water(int isWater_pos_y, int isWater_pos_x) const
{
	const int index = isWater_pos_y * map_width + isWater_pos_x;
	return gsl::span(tiles.get(), map_width * map_height)[index].type == TileType::WATER;
}

void Map::compute_fov()
{
	game.log("Map::compute_fov()");
	if (tcodMap == nullptr)
	{
		game.log("Error: tcodMap is null");
		return;
	}
	tcodMap->computeFov(game.player->posX, game.player->posY, FOV_RADIUS);
}

void Map::render() const
{
	game.log("Map::render()");
	for (int iter_y = 0; iter_y < map_height; iter_y++)
	{
		for (int iter_x = 0; iter_x < map_width; iter_x++)
		{
			if (is_in_fov(iter_x, iter_y) || is_explored(iter_x, iter_y))
			{
				if (is_wall(iter_y, iter_x))
				{
					mvaddch(iter_y, iter_x, '#');
				}
				else if (is_water(iter_y, iter_x))  // Add a function to check if a tile is water
				{
					attron(COLOR_PAIR(WATER_PAIR));
					mvaddch(iter_y, iter_x, '~');
					attroff(COLOR_PAIR(WATER_PAIR));
				}
				else
				{
					mvaddch(iter_y, iter_x, '.');
				}
			}
		}
	}
	refresh(); // Refresh once after all tiles have been drawn
	game.log("Map::render() end");
}

void Map::add_item(int x, int y)
{
	RandomDice d;
	const int dice = d.d100();
	int potionIndex = 0;

	if (dice < 60) { // Adjust this threshold based on your game's item spawn logic

		// Randomly select a weapon from the list
		const int weaponIndex = d.roll(1, game.weapons.size());
		const Weapons& selectedWeapon = game.weapons.at(weaponIndex - 1);

		// Randomly select a color for the weapon
		const int rollColor = d.roll(1,3);

		// Create an Actor for the weapon
		auto weaponActor = std::make_unique<Actor>(x, y, '/', selectedWeapon.name, rollColor, 0); // Assuming you have a generic symbol and color for weapons
		weaponActor->blocks = false;
		//weaponActor->pickable = std::make_shared<Pickable>(selectedWeapon); // Assuming you have a way to handle different weapon types
		if (selectedWeapon.name == "Dagger")
		{
			weaponActor->pickable = std::make_unique<Dagger>(1,4);
			game.err("Dagger added");
		}
		else if (selectedWeapon.name == "Long Sword")
		{
			weaponActor->pickable = std::make_unique<LongSword>(1,8);
			game.err("Long Sword added");
		}
		else
		{
			/*weaponActor->pickable = std::make_shared<Pickable>(selectedWeapon);*/
			game.log("Error: Unknown weapon type"); // Log an error if the weapon type is unknown"
			return;
		}

		game.actors.push_back(std::move(weaponActor));
		auto weaponActorPtr = game.actors.back().get();
		game.send_to_back(*weaponActorPtr);
		
		// add gold 
		auto gold = std::make_unique<Actor>(x, y, '$', "gold", GOLD_PAIR, 0);
		gold->blocks = false;
		gold->pickable = std::make_unique<Gold>(d.roll(1, 10));
		game.actors.push_back(std::move(gold));
		auto goldPtr = game.actors.back().get();
		game.send_to_back(*goldPtr);
	}
	else if (dice < 70)
	{
		// add a health potion
		auto healthPotion = std::make_unique<Actor>(x, y, '!', "health potion", HPBARMISSING_PAIR, 0);
		healthPotion->index = potionIndex++;
		healthPotion->blocks = false;
		healthPotion->pickable = std::make_unique<Healer>(4);
		game.actors.push_back(std::move(healthPotion));
		auto healthPotionPtr = game.actors.back().get();
		game.send_to_back(*healthPotionPtr);
	}
	else if (dice < 70+10)
	{
		// add lightning scrolls
		auto lightningScroll = std::make_unique<Actor>(x, y, '#', "scroll of lightning bolt", LIGHTNING_PAIR,1);
		lightningScroll->blocks = false;
		lightningScroll->pickable = std::make_unique<LightningBolt>(5, 20);
		game.actors.push_back(std::move(lightningScroll));
		auto lightningScrollPtr = game.actors.back().get();
		game.send_to_back(*lightningScrollPtr);
	}
	else if (dice < 70 + 10 + 10)
	{
		// add fireball scrolls
		auto fireballScroll = std::make_unique<Actor>(x, y, '#', "scroll of fireball", FIREBALL_PAIR,1);
		fireballScroll->blocks = false;
		fireballScroll->pickable = std::make_unique<Fireball>(3, 12);
		game.actors.push_back(std::move(fireballScroll));
		auto fireballScrollPtr = game.actors.back().get();
		game.send_to_back(*fireballScrollPtr);
	}
	else
	{
		// add confusion scrolls
		auto confusionScroll = std::make_unique<Actor>(x, y, '#', "scroll of confusion", CONFUSION_PAIR,0);
		confusionScroll->blocks = false;
		confusionScroll->pickable = std::make_unique<Confuser>(10, 8);
		game.actors.push_back(std::move(confusionScroll));
		auto confusionScrollPtr = game.actors.back().get();
		game.send_to_back(*confusionScrollPtr);
	}
}

void Map::dig(int x1, int y1, int x2, int y2)
{
	if (x2 < x1)
	{
		const int tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y2 < y1)
	{
		const int tmp = y2;
		y2 = y1;
		y1 = tmp;
	}

	for (int tileY = y1; tileY <= y2 +1; tileY++)
	{
		for (int tileX = x1; tileX <= x2 +1; tileX++)
		{
			tcodMap->setProperties(
				tileX,
				tileY,
				true,
				true
			);			
		}
	}
}

void Map::set_tile(int x, int y, TileType newType) noexcept
{
	if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
	{
		// Coordinates out of bounds, don't do anything
		return;
	}

	// Calculate the index into the array
	const int index = y * MAP_WIDTH + x;

	// Make a span from the tiles array
	const gsl::span<Tile> tilesSpan(tiles.get(), MAP_WIDTH * MAP_HEIGHT);

	// Use the span for access
	tilesSpan[index].type = newType;
}

void Map::create_room(bool first, int x1, int y1, int x2, int y2, bool withActors)
{
	RandomDice d;
	dig(x1, y1, x2, y2); // dig the corridors

	// Add water tiles
	constexpr int waterPercentage = 10; // 10% of tiles will be water, adjust as needed
	for (int x = x1; x <= x2; x++)
	{
		for (int y = y1; y <= y2; y++)
		{
			const int rolld100 = d.d100();
			if (rolld100 < waterPercentage)
			{
				// Assuming you have a set_tile function that sets the tile at (x, y) to water
				set_tile(x, y, TileType::WATER);
			}
		}
	}

	if (!withActors)
	{
		return;
	}

	if (first) // if this is the first room, we need to place the player in it
	{
		game.player->posY = y1 + 1;
		game.player->posX = x1 + 1;

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
			const int monsterX = TCODRandom::getInstance()->getInt(x1, x2);
			const int monsterY = TCODRandom::getInstance()->getInt(y1, y2);

			if (is_wall(monsterY, monsterX))
			{
				continue;
			}

			add_monster(monsterX, monsterY);
		}

		// add stairs
		game.stairs->posX = x1 + 1;
		game.stairs->posY = y1 + 1;
	} 
	
	// add items
	const int numItems = TCODRandom::getInstance()->getInt(0, MAX_ROOM_ITEMS);

	for (int i = 0; i < numItems; i++)
	{
		const int itemX = TCODRandom::getInstance()->getInt(x1, x2);
		const int itemY = TCODRandom::getInstance()->getInt(y1, y2);

		if (is_wall(itemY, itemX))
		{
			continue;
		}

		add_item(itemY, itemX);
	}

}

//====
// We want to detect, when the player tries to walk on a Tile , if it is occupied by another actor.
bool Map::can_walk(int canw_x, int canw_y) const
{
	
	if (is_wall(canw_y, canw_x)) // check if the tile is a wall
	{
		return false;
	}
	if (is_water(canw_y, canw_x)) // check if the tile is water
	{
		return false;
	}

	for (const auto& actor : game.actors) // iterate through the actor deque
	{
		if (
			actor->blocks // if the actor blocks
			&& 
			actor->posX == canw_x // and if there is another actor on the tile
			&& 
			actor->posY == canw_y
			)
		{
			return false;
		}
	}
	
	return true;
}

void Map::add_monster(int mon_x, int mon_y)
{
	game.err("player level: " + std::to_string(game.player->playerLevel));
	static bool dragonPlaced = false; // flag to track if a dragon has been placed
	RandomDice d;

	// Determine if this room should contain the dragon
	const bool placeDragon = !dragonPlaced && d.d100() < 5; // 5% chance to place a dragon

	if (placeDragon)
	{
		auto dragon = std::make_unique<Dragon>(mon_y, mon_x);
		dragon->index = 0;
		game.actors.push_back(std::move(dragon));
		dragonPlaced = true; // set the flag to true so no more dragons are placed
	}
	else
	{
		const auto roll4d6 = d.d6() + d.d6() + d.d6() + d.d6(); // roll 4d6
		for (auto i{ 0 }; i < roll4d6; i++) { // create goblins
			auto goblin = create_monster<Goblin>(mon_y, mon_x);
			goblin->index = i;
			game.actors.push_back(std::move(goblin));
		}

		if (game.player->playerLevel > 3)
		{
			const auto roll2d6 = d.d6() + d.d6(); // roll 2d6
			for (auto i{ 0 }; i < roll2d6; i++) { // create orcs
				auto orc = create_monster<Orc>(mon_y, mon_x);
				orc->index = i;
				game.actors.push_back(std::move(orc));
			}
		}

		if (game.player->playerLevel > 5)
		{
			const auto roll1d6 = d.d6();
			for (auto i{ 0 }; i < roll1d6; i++) { // create trolls
				auto troll = create_monster<Troll>(mon_y, mon_x);
				troll->index = i;
				game.actors.push_back(std::move(troll));
			}
		}
	}
}

Actor* Map::get_actor(int x, int y) noexcept
{
	auto it = std::find_if(
		game.actors.begin(),
		game.actors.end(),
		[&](const auto& actor) noexcept
		{
			return actor->posX == x && actor->posY == y;
		}
	);

	if (it != game.actors.end()) {
		return it->get();
	}

	return nullptr;
}

// end of file: Map.cpp
