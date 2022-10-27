#include <iostream>

#include "main.h"
#include "Colors.h"

//====
//a constexpr for tracking the number of turns
constexpr auto TRACKING_TURNS = 3;

//==AI==
Ai* Ai::create(TCODZip& zip) 
{
	AiType type = (AiType)zip.getInt();
	Ai* ai = nullptr;

	switch (type) 
	{
		
	case AiType::PLAYER:
	{
		ai = new PlayerAi();
		break;
	}

	case AiType::MONSTER:
	{
		ai = new MonsterAi(); 
		break;
	}
	
	case AiType::CONFUSED_MONSTER:
	{
		ai = new ConfusedMonsterAi(0, nullptr);
		break;
	}

	}

	ai->load(zip);
	return ai;
}

//==MONSTER_AI==
MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor* owner)
{

	if (owner->destructible && owner->destructible->is_dead()) // if the owner is dead
	{
		return; // do nothing
	}

	if (engine.map->is_in_fov(owner->posX, owner->posY)) // if the owner is in the fov
	{
		// move towards the player
		moveCount = TRACKING_TURNS;
	}
	else
	{
		moveCount--; // decrement the move count
	}

	if (moveCount > 0) // if the move count is greater than 0
	{
		moveOrAttack(owner, engine.player->posX, engine.player->posY); // move or attack the player
	}
}

void MonsterAi::load(TCODZip& zip)
{
	moveCount = zip.getInt();
}

void MonsterAi::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::MONSTER));
	zip.putInt(moveCount);
}

//====
// how many turns the monster chases the player
// after losing his sight
void MonsterAi::moveOrAttack(Actor* owner, int targetx, int targety)
{
	int dx = targetx - owner->posX; // get the x distance
	int dy = targety - owner->posY; // get the y distance

	int stepdx = (dx > 0 ? 1 : -1); // get the x step
	int stepdy = (dy > 0 ? 1 : -1); // get the y step

	double distance = sqrt(dx * dx + dy * dy); // get the distance

	if (distance >= 2)
	{
		dx = static_cast<int>(round(dx / distance));
		dy = static_cast<int>(round(dy / distance));

		if (engine.map->can_walk(owner->posX + dx, owner->posY + dy))
		{
			owner->posX += dx;
			owner->posY += dy;
		}
		else if (engine.map->can_walk(owner->posX + stepdx, owner->posY))
		{
			owner->posX += stepdx;
		}
		else if (engine.map->can_walk(owner->posX, owner->posY + stepdy))
		{
			owner->posY += stepdy;
		}
	}

	else if (owner->attacker)
	{
		owner->attacker->attack(owner, engine.player);
	}
}

//==PLAYER_AI==

//==CONTROLS==
// the enumeration for the controls of the player
enum class Controls : int
{
	// the controls for the player movement
	UP = '8',
	UP_ARROW = KEY_UP,
	UP_ARROW_NUMPAD = KEY_A2,

	DOWN = '2',
	DOWN_ARROW = KEY_DOWN,
	DOWN_ARROW_NUMPAD = KEY_C2,

	LEFT = '4',
	LEFT_ARROW = KEY_LEFT,
	LEFT_ARROW_NUMPAD = KEY_B1,

	RIGHT = '6',
	RIGHT_ARROW = KEY_RIGHT,
	RIGHT_ARROW_NUMPAD = KEY_B3,

	UP_LEFT = '7',
	UP_LEFT_ARROW_NUMPAD = KEY_A1,

	UP_RIGHT = '9',
	UP_RIGHT_ARROW_NUMPAD = KEY_A3,

	DOWN_LEFT = '1',
	DOWN_LEFT_ARROW_NUMPAD = KEY_C1,

	DOWN_RIGHT = '3',
	DOWN_RIGHT_ARROW_NUMPAD = KEY_C3,

	// input for the player to wait
	WAIT = '5',
	WAIT_ARROW_NUMPAD = KEY_B2,

	// input for the player to hit himself
	HIT_SELF = ' ',

	// input for the player to pick items 
	PICK = 'p',
	PICK_SHIFT_STAR = '*',
	PICK_NUMPAD = PADSTAR,

	// input for displaying the inventory
	INVENTORY = 'i',

	// input for displaying the game menu
	ESCAPE = 27,
	
	MOUSE = KEY_MOUSE,

	HEAL = 'a',
	
	// input for the player to drop items
	DROP = 'd',

	CHAR_SHEET = 'c',

	DESCEND = '>',

	// input for the player to target 
	TARGET = 't',

	// the controls for the player to exit the game
	QUIT = 'q'
};

PlayerAi::PlayerAi() : xpLevel(1) {}

const int LEVEL_UP_BASE = 200;
const int LEVEL_UP_FACTOR = 150;

int PlayerAi::getNextLevelXp()
{
	return LEVEL_UP_BASE + (xpLevel * LEVEL_UP_FACTOR);
}

//template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
//int putEnum(T value) { putInt(static_cast<int>(value)); }
//
//template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
//T getEnum() { return static_cast<T>(getInt()); }

template<typename T>
T castEnum(T value) { return static_cast<T>(value); }

auto exampleInt = castEnum(Controls::UP);

void PlayerAi::update(Actor* owner)
{	
	int levelUpXp = getNextLevelXp();
	if (owner->destructible->xp >= levelUpXp) 
	{
		xpLevel++;
		owner->destructible->xp -= levelUpXp;
		engine.gui->log_message(WHITE_PAIR, "Your battle skills grow stronger! You reached level %d", xpLevel);
		engine.dispay_stats(xpLevel);
	}

	if (owner->destructible && owner->destructible->is_dead())
	{
		return;
	}
	
	int dx = 0, dy = 0;
	
	clear();

	switch (static_cast<Controls>(engine.keyPress))
	{
	case Controls::UP:
	case Controls::UP_ARROW:
	case Controls::UP_ARROW_NUMPAD:
		dy = -1;
		break;
	case Controls::DOWN:
	case Controls::DOWN_ARROW:
	case Controls::DOWN_ARROW_NUMPAD:
		dy = 1;
		break;
	case Controls::LEFT:
	case Controls::LEFT_ARROW:
	case Controls::LEFT_ARROW_NUMPAD:
		dx = -1;
		break;
	case Controls::RIGHT:
	case Controls::RIGHT_ARROW:
	case Controls::RIGHT_ARROW_NUMPAD:
		dx = 1;
		break;
	case Controls::UP_LEFT:
	case Controls::UP_LEFT_ARROW_NUMPAD:
		dx = -1;
		dy = -1;
		break;
	case Controls::UP_RIGHT:
	case Controls::UP_RIGHT_ARROW_NUMPAD:
		dx = 1;
		dy = -1;
		break;
	case Controls::DOWN_LEFT:
	case Controls::DOWN_LEFT_ARROW_NUMPAD:
		dx = -1;
		dy = 1;
		break;
	case Controls::DOWN_RIGHT:
	case Controls::DOWN_RIGHT_ARROW_NUMPAD:
		dx = 1;
		dy = 1;
		break;
	case Controls::WAIT:
	case Controls::WAIT_ARROW_NUMPAD:
		engine.gameStatus = Engine::GameStatus::NEW_TURN;
		break;
	case Controls::HIT_SELF:
		engine.player->attacker->attack(engine.player, engine.player);
		break;
	case Controls::MOUSE:
		std::cout << "mouse" << std::endl;
		request_mouse_pos();
		break;
	case Controls::PICK:
	case Controls::PICK_SHIFT_STAR:
	case Controls::PICK_NUMPAD:
		pick_item(owner);
		break;
	case Controls::DROP:
	{
		Actor* actor = choseFromInventory(owner, 'a');
		if (actor) 
		{
			actor->pickable->drop(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	break;
	case Controls::INVENTORY:
		/*handleActionKey(owner, engine.keyPress);*/
		display_inventory(owner);
		break;
	case Controls::QUIT:
		engine.run = false;
		if (engine.run == false)
		{
			mvprintw(29, 0, "You quit the game ! Press any key ...");
		}
		break;
	// if escape key is pressed bring the game menu
	case Controls::ESCAPE:
		engine.game_menu();
		break;

	case Controls::DESCEND:
		if (engine.stairs->posX == owner->posX && engine.stairs->posY == owner->posY)
		{
			engine.next_level();
		}
		break;
	case Controls::TARGET:
		engine.target();
		break;
	case Controls::CHAR_SHEET:
		engine.display_character_sheet();
		break;
	
	default:break;
	}

	if (dx!=0||dy!=0)
	{
		engine.gameStatus = Engine::GameStatus::NEW_TURN;
		if (moveOrAttack(owner,owner->posX+dx,owner->posY+dy))
		{
			engine.map->compute_fov();
		}
	}
}

void PlayerAi::pick_item(Actor* owner)
{
	std::clog << "You pick" << std::endl;
	bool found = false;
	for (Actor* actor : engine.actors)
	{
		if (
			actor->pickable
			&&
			actor->posX == owner->posX
			&&
			actor->posY == owner->posY
			)
		{
			if (actor->pickable->pick(actor, owner))
			{
				found = true;
				engine.gui->log_message(DARK_GROUND_PAIR, "You take the %s.", actor->name);
				break;
			}
			else if (!found)
			{
				found = true;
				engine.gui->log_message(HPBARMISSING_PAIR, "Your inventory is full.");
			}
		}
	}

	if (!found)
	{
		engine.gui->log_message(HPBARFULL_PAIR, "There is nothing to pick up.");
	}

	engine.gameStatus = Engine::GameStatus::NEW_TURN;
}

void PlayerAi::display_inventory(Actor* owner)
{
	/*Actor* actor = choseFromInventory(owner, ascii);*/

	constexpr int INVENTORY_HEIGHT = 29;
	constexpr int INVENTORY_WIDTH = 30;

	refresh();

	WINDOW* inv = newwin(
		INVENTORY_HEIGHT, // int nlines
		INVENTORY_WIDTH, // int ncols
		0, // int begy
		0 // int begx
	);

	// display the inventory frame
	/*con.setDefaultForeground(TCODColor(200, 180, 50));*/
	/*con.printFrame(0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT, true, TCOD_BKGND_DEFAULT, "inventory");*/
	box(inv, 0, 0);
	mvwprintw(inv, 0, 0, "Inventory");

	// display the items with their keyboard shortcut
	/*con.setDefaultForeground(TCODColor::white);*/
	int shortcut = 'a';
	int y = 1;
	/*for (Actor** it = owner->container->inventory.begin();
		it != owner->container->inventory.end(); it++) {
		Actor* actor = *it;
		con.print(2, y, "(%c) %s", shortcut, actor->name);
		y++;
		shortcut++;
	}*/

	for (Actor* actor : owner->container->inventoryList)
	{
		/*engine.gui->log_message(y, 0, "(%c) %s", shortcut, actor->name);*/

		mvwprintw(inv, y, 1, "(%c) %s", shortcut, actor->name);
		y++;
		shortcut++;
	}

	/*// blit the inventory console on the root console
	TCODConsole::blit(&con, 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT,
		TCODConsole::root, engine.screenWidth / 2 - INVENTORY_WIDTH / 2,
		engine.screenHeight / 2 - INVENTORY_HEIGHT / 2);
	TCODConsole::flush();*/
	wrefresh(inv);

	//std::clog << "You display inventory" << std::endl;
	//if (actor)
	//{
	//	actor->pickable->use(actor, owner);
	//}

	Gui gui;
	// wait for a key press
	int inventoryInput = getch();
	switch (inventoryInput)
	{
	case static_cast<int>(Controls::ESCAPE):
		clear();
		break;

	case 'a':
	{
		// use item
		Actor * actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'b':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'c':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'd':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'e':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}

	case 'f':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}

	case 'h':
	{
		// use item
		Actor* actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}

	default:
		clear();
		break;
	}
}

Actor* PlayerAi::choseFromInventory(Actor* owner, int ascii)
{
	if (ascii == 'a')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[0];
		}
	}
	if (ascii == 'b')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[1];
		}
	}
	if (ascii == 'c')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[2];
		}
	}
	if (ascii == 'd')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[3];
		}
	}
	if (ascii == 'e')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[4];
		}
	}
	if (ascii == 'f')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[5];
		}
	}
	if (ascii == 'g')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[6];
		}
	}
	// if case is 'h'
	if (ascii == 'h')
	{
		if (owner->container->inventoryList.size() > 0)
		{
			return owner->container->inventoryList[7];
		}
	}

	return nullptr;
}

void PlayerAi::load(TCODZip& zip)
{
}

void PlayerAi::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::PLAYER));
}

bool PlayerAi::moveOrAttack(Actor* owner, int targetx, int targety) 
{
	if (engine.map->is_wall(targety, targetx))
	{
		return false;
	}

	// look for living actors to attack
	for (const auto& actor : engine.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->is_dead()
			&&
			actor->posX == targetx
			&&
			actor->posY == targety
			)
		{
			owner->attacker->attack(owner, actor);
			return false;
		}
	}

	// look for corpses or items
	for (const auto& actor : engine.actors)
	{
		bool corpseOrItem = (
			actor->destructible
			&&
			actor->destructible->is_dead()
			)
			||
			actor->pickable;

		if (
			corpseOrItem
			&&
			//actor->destructible 
			//&& 
			//actor->destructible->isDead()
			//&& 
			actor->posX == targetx
			&&
			actor->posY == targety
			)
		{
		//std::cout <<
		//	"There's a %s here\n"
		//	<< std::endl;

			mvprintw(
				29,
				0,
				"There's a %s here\n",
				actor->name
			);
		}
	}

		owner->posX = targetx;
		owner->posY = targety;

		return true;
}

//==ConfusedMonsterAi==
ConfusedMonsterAi::ConfusedMonsterAi(int nbTurns, Ai* oldAi) :
	nbTurns(nbTurns),
	oldAi(oldAi)
{
}

void ConfusedMonsterAi::update(Actor* owner)
{
	TCODRandom* rng = TCODRandom::getInstance();
	int dx = rng->getInt(-1, 1);
	int dy = rng->getInt(-1, 1);

	if (dx != 0 || dy != 0)
	{
		int destx = owner->posX + dx;
		int desty = owner->posY + dy;

		if (engine.map->can_walk(desty, destx))
		{
			owner->posX = destx;
			owner->posY = desty;
		}
		else
		{
			Actor* actor = engine.get_actor(destx, desty);
			if (actor)
			{
				owner->attacker->attack(owner, actor);
			}
		}
	}

	nbTurns--;
	if (nbTurns == 0)
	{
		owner->ai = oldAi;
		delete this;
	}
}

void ConfusedMonsterAi::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	oldAi = Ai::create(zip);
}

void ConfusedMonsterAi::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::CONFUSED_MONSTER));
	zip.putInt(nbTurns);
	oldAi->save(zip);
}
//====