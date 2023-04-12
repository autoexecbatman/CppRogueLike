#include <curses.h>
#include <random>
#include <iostream>
//#include "main.h"

//#include "libtcod.hpp"
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
class Actor;
#include "Persistent.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Actor.h"
#include "Map.h"
#include "Game.h"

#include "Colors.h"

#include "Goblin.h"

#include "globals.h"

//#include "/Repositories/CaptainCrowbar/dice/source/dice/dice.hpp"
//#include "/Repositories/CaptainCrowbar/dice/source/dice/rational.hpp"

constexpr auto ROOM_MAX_SIZE = 12;
constexpr auto ROOM_MIN_SIZE = 6;
constexpr auto MAX_ROOM_MONSTERS = 3;
constexpr int MAX_ROOM_ITEMS = 2;

// a binary space partition listener class (BSP)
class BspListener : public ITCODBspCallback
{
private:
	Map& map; // a map to dig
	int roomNum; // room number
	int lastx = 0, lasty = 0; // center of the last room
public:
	BspListener(Map& map) : map(map), roomNum(0) {}

	bool visitNode(TCODBsp* node, void* userData)
	{
		if (node->isLeaf())
		{
			// variables
			int room_pos_x = 0, room_pos_y = 0, room_width = 0, room_height = 0;
			bool withActors = (bool)userData;
			// dig a room
			/*TCODRandom* rng = TCODRandom::getInstance();*/
			room_width = map.rng->getInt(ROOM_MIN_SIZE, node->w - 2);//random int from min size to width - 2
			room_height = map.rng->getInt(ROOM_MIN_SIZE, node->h - 2);//random int from min size to height - 2
			room_pos_x = map.rng->getInt(node->x + 1, node->x + node->w - room_width - 1);//from node x + 1 to node x + node width - width - 1
			room_pos_y = map.rng->getInt(node->y + 1, node->y + node->h - room_height - 1);//from node y + 1 to node x + node height - width - 1

			map.create_room//create rooms func
			(
				roomNum == 0,
				room_pos_x,
				room_pos_y,
				room_pos_x + room_width - 1 - 1,
				room_pos_y + room_height - 1 - 1,
				withActors
			);
			
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
				/*map.dig(1,10,117,10);*/
			}
			//set variables
			lastx = room_pos_x + room_width / 2;
			lasty = room_pos_y + room_height / 2;
			
			//iterate to next room
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
		tiles[i].explored = zip.getInt();
	}
}

void Map::save(TCODZip& zip)
{
	zip.putInt(seed);
	for (int i = 0; i < map_width * map_height; i++) 
	{
		zip.putInt(tiles[i].explored);
	}
}

void Map::bsp(int map_width, int map_height, TCODRandom* rng, bool withActors)
{
	
	TCODBsp* myBSP = new TCODBsp(0, 0, map_width, map_height);
	myBSP->splitRecursive(rng, 4, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
	BspListener* mylistener = new BspListener(*this);
	myBSP->traverseInvertedLevelOrder(mylistener, (void*)withActors);
}

//====
//In Map.cpp, we allocate the TCODMap object in the constructor
Map::Map(
	int map_height,
	int map_width
)
	:
	map_height(map_height),
	map_width(map_width),
	tcodMap(nullptr),
	rng(nullptr),
	tiles(nullptr)
{
	// a random seed for the map
	seed = TCODRandom::getInstance()->getInt(0, 0x7FFFFFFF); // 0x7FFFFFFF is the highest possible 32 bit signed integer value.
}

Map::~Map()
{
	delete[] tiles; // free the map's tiles
	delete tcodMap; //delete the TCODMap object
}

bool Map::is_wall(int isWall_pos_y, int isWall_pos_x) const // checks if it is a wall?
{
	// return !tiles[isWall_pos_x + isWall_pos_y * map_width].canWalk;

	return !tcodMap->isWalkable(
		isWall_pos_x,
		isWall_pos_y
	);
}

bool Map::is_explored(int exp_x, int exp_y) const
{
	return tiles[exp_x + (exp_y * map_width)].explored;
}

bool Map::is_in_fov(int fov_x, int fov_y) const
{
	// if tcodMap is null return false
	if (tcodMap != nullptr)
	{


		if ( // fov is out of bounds
			fov_x < 0
			||
			fov_x >= map_width
			||
			fov_y < 0
			||
			fov_y >= map_height
			)
		{
			return false;
		}
		if (tcodMap->isInFov(fov_x, fov_y))
		{
			tiles[fov_x + fov_y * map_width].explored = true;
			return true;
		}
		return false;
	}
	else
	{
		std::clog << "tcodMap is null !!!" << std::endl;
		std::cout << "tcodMap is null !!!" << std::endl;
		exit(-1);
	}
}

void Map::compute_fov()
{
	std::clog << "Map::compute_fov()" << std::endl;
	tcodMap->computeFov(game.player->posX, game.player->posY, 10);
}

void Map::render() const
{
	std::clog << "Map::render()" << std::endl;
	for (int iter_y = 0; iter_y < map_height; iter_y++)
	{
		for (int iter_x = 0; iter_x < map_width; iter_x++)
		{
			if (is_in_fov(iter_x, iter_y))
			{
				//int key = getch();
				//if (key == 't')
				//{
					if (is_wall(iter_y, iter_x))
					{
						// TODO : add a key to toggle between fovVisible on and off
						/*standout();*/
						mvaddch(iter_y, iter_x, '#');
						/*standend();*/

					}
					else
					{
						/*standout();*/
						mvaddch(iter_y, iter_x, '.');
						/*standend();*/
					}
				//}
				/*mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? LIGHT_WALL_PAIR : LIGHT_GROUND_PAIR, NULL);*/
			}
			else if (is_explored(iter_x,iter_y))
			{
				if (is_wall(iter_y,iter_x))
					{
					mvaddch(iter_y, iter_x, '#');
					}
				else
					{
					mvaddch(iter_y, iter_x, '.');
					}
				
				/*mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? DARK_WALL_PAIR : DARK_GROUND_PAIR, NULL);*/
			}
			/*mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? darkWall : darkGround, NULL);*/
		}
	}
	std::clog << "Map::render() end" << std::endl;
}

void Map::add_item(int x, int y)
{
	TCODRandom* rng = TCODRandom::getInstance();
	int dice = rng->getInt(0, 100);
	
	static int potionIndex = 0;
	if (dice < 70)
	{
		// add health potion
		/*Actor* healthPotion = new Actor(x, y, '!', "health potion", HPBARMISSING_PAIR);*/
		auto healthPotion = std::make_shared<Actor>(x, y, '!', "health potion", HPBARMISSING_PAIR, 0);
		healthPotion->index = potionIndex++;
		healthPotion->blocks = false;
		/*healthPotion->pickable = new Healer(4);*/
		healthPotion->pickable = std::make_shared<Healer>(4);
		game.actors.push_back(healthPotion);
		/*game.actors.emplace_back(healthPotion);*/
		/*game.actors.emplace(healthPotion);*/
		/*game.actors.try_emplace(healthPotion->index,healthPotion);*/
		//auto result = game.actors.insert({ healthPotion->index, healthPotion });
		//if (!result.second) {
		//	// an element with the same key already exists
		//	// you may want to handle this error case
		//	// for example, by deleting the healthPotion shared_ptr
		//}

		// Generate a new index for the potion

		// Add the potion to the map
		/*game.actors.try_emplace(healthPotion->index, healthPotion);*/

		game.send_to_back(*healthPotion);
	}
	else if (dice < 70+10)
	{
		// add lightning scrolls
		/*Actor* lightningScroll = new Actor(x, y, '#', "scroll of lightning bolt", LIGHTNING_PAIR);*/
		auto lightningScroll = std::make_shared<Actor>(x, y, '#', "scroll of lightning bolt", LIGHTNING_PAIR,1);
		lightningScroll->blocks = false;
		/*lightningScroll->pickable = new LightningBolt(5, 20);*/
		lightningScroll->pickable = std::make_shared<LightningBolt>(5, 20);
		/*game.actors.emplace_back(lightningScroll);*/
		/*game.actors.emplace(lightningScroll);*/
		/*game.send_to_back(lightningScroll);*/
	}
	else if (dice < 70 + 10 + 10)
	{
		// add fireball scrolls
		/*Actor* fireballScroll = new Actor(x, y, '#', "scroll of fireball", FIREBALL_PAIR);*/
		auto fireballScroll = std::make_shared<Actor>(x, y, '#', "scroll of fireball", FIREBALL_PAIR,1);
		fireballScroll->blocks = false;
		/*fireballScroll->pickable = new Fireball(3, 12);*/
		fireballScroll->pickable = std::make_shared<Fireball>(3, 12);
		game.actors.push_back(fireballScroll);
		/*game.actors.emplace_back(fireballScroll);*/
		/*game.actors.emplace(fireballScroll);*/
		
		/*game.actors.insert(std::make_pair(fireballScroll->index, fireballScroll));*/
		/*game.send_to_back(fireballScroll);*/
	}
	else
	{
		// add confusion scrolls
		/*Actor* confusionScroll = new Actor(x, y, '#', "scroll of confusion", CONFUSION_PAIR);*/
		auto confusionScroll = std::make_shared<Actor>(x, y, '#', "scroll of confusion", CONFUSION_PAIR,0);
		confusionScroll->blocks = false;
		/*confusionScroll->pickable = new Confuser(10, 8);*/
		confusionScroll->pickable = std::make_shared<Confuser>(10, 8);
		/*game.actors.emplace_back(confusionScroll);*/
		/*game.actors.emplace(confusionScroll);*/
		/*game.send_to_back(confusionScroll);*/
	}
	/*else if (dice < 70 + 10 + 10)*/
	// always spawn this scroll
	//if (true)
	//{
	//	// add fireball scrolls
	//	Actor* fireballScroll = new Actor(x, y, '#', "scroll of fireball", FIREBALL_PAIR);
	//	fireballScroll->blocks = false;
	//	fireballScroll->pickable = new Fireball(3, 12);
	//	game.actors.push_back(fireballScroll);
	//	game.sendToBack(fireballScroll);
	//}
	//
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors)
{
	rng = new TCODRandom(seed, TCOD_RNG_CMWC);
	tiles = new Tile[map_height * map_width]; // allocate the map's tiles
	tcodMap = new TCODMap(map_width, map_height); // allocate the map
	bsp(map_width, map_height, rng, withActors);
}

void Map::dig(int x1, int y1, int x2, int y2)
{
	if (x2 < x1)
	{
		int tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y2 < y1)
	{
		int tmp = y2;
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

void Map::create_room(bool first, int x1, int y1, int x2, int y2, bool withActors)
{
	dig(x1, y1, x2, y2); // dig the corridors

	if (!withActors)
	{
		return;
	}

	if (first) // if this is the first room, we need to place the player in it
	{
		game.player->posY = y1 + 1;
		game.player->posX = x1 + 1;
		
		//// create player 2
		//game.player2->posY = y1 + 2;
		//game.player2->posX = x1 + 1;
		//
		//// create player 3
		//game.player3->posY = y1 + 3;
		//game.player3->posX = x1 + 1;
	}
	
	// If this is NOT the first room, we make a random number of monsters and place them in the room
	// First we get a random number of monsters and for each one, get a random position inside the room.
	// If the tile is empty (canWalk) we create a monster.
	
	else
	{
		int numMonsters = TCODRandom::getInstance()->getInt(0, MAX_ROOM_MONSTERS);
		for (int i = 0; i < numMonsters; i++)
		{
			int monsterX = TCODRandom::getInstance()->getInt(x1, x2);
			int monsterY = TCODRandom::getInstance()->getInt(y1, y2);

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
	int numItems = TCODRandom::getInstance()->getInt(0, MAX_ROOM_ITEMS);

	for (int i = 0; i < numItems; i++)
	{
		int itemX = TCODRandom::getInstance()->getInt(x1, x2);
		int itemY = TCODRandom::getInstance()->getInt(y1, y2);

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

struct Dice{
	int nb_rolls;
	int nb_faces;
	int bonus;
	Dice(int nb_rolls, int nb_faces, int bonus) : nb_rolls(nb_rolls), nb_faces(nb_faces), bonus(bonus) {}
};
// make dice literal 
Dice operator "" _d(unsigned long long nb_faces){
	return Dice(1, static_cast<int>(nb_faces), 0);
}
// overload + operator for dice and int
Dice operator+(const Dice& d1, int bonus){
	return Dice(d1.nb_rolls, d1.nb_faces, d1.bonus + bonus);
}
void test_dice(){
	Dice d1 = 6_d + 4;
	Dice d2 = 6_d + 4 + 2;
	Dice d3 = 6_d + 4 + 2 + 3;
	// print the dice header using mvprintw using log_message function
	game.gui->log_message(WHITE_PAIR, "Dice: %dD%d+%d = %d", d1.nb_rolls, d1.nb_faces, d1.bonus, d1);
	game.gui->log_message(WHITE_PAIR, "Dice: %dD%d+%d = %d", d2.nb_rolls, d2.nb_faces, d2.bonus, d2);
	game.gui->log_message(WHITE_PAIR, "Dice: %dD%d+%d = %d", d3.nb_rolls, d3.nb_faces, d3.bonus, d3);
}

void Map::add_monster(int mon_x, int mon_y){
	// TODO : a 4 tile long monstrous dragon "Dogo" with 1000 HP and 1000 damage
	// TODO : a 1 tile long "Goblin" with 10 HP and 10 damage
	RandomDice d;
	//create a random amount of orcs and trolls in the room based on a d100
	for (int i = 0; i < 4 * d.d6(); i++){
		Goblin goblin(mon_y, mon_x);
		/*game.actors.emplace_back(goblin.create_goblin(mon_y, mon_x));*/
		/*game.actors.try_emplace(i,goblin.create_goblin(mon_y, mon_x));*/
		/*std::shared_ptr<Actor> actor = std::static_pointer_cast<Actor>(goblin.create_goblin(mon_y, mon_x));*/
		auto actor = goblin.create_goblin(mon_y, mon_x);

		/*game.actors.try_emplace(i, actor);*/
		game.actors.push_back(actor);
		// store the key i in the actor index
		actor->index = i;
	}
	//for (int i = 0; i < 2 * d.d6(); i++){
	//	Orc orc(mon_y, mon_x);
	//	/*game.actors.emplace_back(orc.create_orc(mon_y, mon_x));*/
	//	game.actors.try_emplace(i ,orc.create_orc(mon_y, mon_x));
	//}
	//for (int i = 0; i < 2 * d.d6(); i++){
	//	Troll troll(mon_y, mon_x);
	//	/*game.actors.emplace_back(troll.create_troll(mon_y, mon_x));*/
	//	/*game.actors.emplace(troll.create_troll(mon_y, mon_x));*/
	//	game.actors.try_emplace(i, troll.create_troll(mon_y, mon_x));
	//}
	//for (int i = 0; i < 1 * d.d6(); i++){
	//	Dragon dragon(mon_y, mon_x);
	//	/*game.actors.emplace_back(dragon.create_dragon(mon_y, mon_x));*/
	//	/*game.actors.emplace(dragon.create_dragon(mon_y, mon_x));*/
	//}
}

std::shared_ptr<Actor> Map::get_actor(int x, int y) const
{
	for (const auto& actor : game.actors)
	{
		if (actor->posX == x && actor->posY == y)
		{
			return actor;
		}
	}
	return nullptr;
}

// make a random number function to use in the game
int Map::random_number(int min, int max)
{
	static std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
	std::uniform_int_distribution<int> range(min, max);

	return range(randomEngine);
}

//====
