// file: Ai.cpp
#include <iostream>
#include <curses.h>
#include <gsl/util>
#include <gsl/pointers>

#include "main.h"
#include "Menu.h"
#include "Colors.h"

//====
//a constexpr for tracking the number of turns
constexpr auto TRACKING_TURNS = 3;

//==AI==
std::shared_ptr<Ai> Ai::create(TCODZip& zip) 
{
	AiType type = (AiType)zip.getInt();
	/*Ai* ai = nullptr;*/
	std::shared_ptr<Ai> ai = nullptr;

	switch (type) 
	{

	case AiType::PLAYER:
	{
		/*ai = new PlayerAi();*/
		ai = std::make_shared<PlayerAi>();
		break;
	}

	case AiType::MONSTER:
	{
		/*ai = new MonsterAi();*/
		ai = std::make_shared<MonsterAi>();
		break;
	}
	
	case AiType::CONFUSED_MONSTER:
	{
		/*ai = new ConfusedMonsterAi(0, nullptr);*/
		ai = std::make_shared<ConfusedMonsterAi>(0, nullptr);
		break;
	}

	}

	if (ai != nullptr)
	{
	ai->load(zip);
	return ai;
}
	else
	{
		std::cout << "Error: Ai::create() - ai is null" << std::endl;
		exit(-1);
	}

	return ai; // TODO: don't return nullptr
}

//==MONSTER_AI==
MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor& owner)
{
	if (owner.ai == nullptr) // if the owner has no ai
	{
		return; // do nothing
	}

	if (owner.destructible != nullptr)
	{
		if (owner.destructible->is_dead()) // if the owner is dead
		{
		return; // do nothing
	}
	}
	else
	{
		std::cout << "Error: MonsterAi::update() - owner.destructible is null" << std::endl;
		exit(-1);
	}

	if (game.map->is_in_fov(owner.posX, owner.posY)) // if the owner is in the fov
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
		moveOrAttack(owner, game.player->posX, game.player->posY); // move or attack the player
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
void MonsterAi::moveOrAttack(Actor& owner, int targetx, int targety)
{
	int dx = targetx - owner.posX; // get the x distance
	int dy = targety - owner.posY; // get the y distance

	int stepdx = (dx > 0 ? 1 : -1); // get the x step
	int stepdy = (dy > 0 ? 1 : -1); // get the y step

	double distance = sqrt(dx * dx + dy * dy); // get the distance

	if (distance >= 2)
	{
		dx = static_cast<int>(round(dx / distance));
		dy = static_cast<int>(round(dy / distance));

		if (game.map != nullptr)
		{
		if (game.map->can_walk(owner.posX + dx, owner.posY + dy))
		{
			owner.posX += dx;
			owner.posY += dy;
		}
		else if (game.map->can_walk(owner.posX + stepdx, owner.posY))
		{
			owner.posX += stepdx;
		}
		else if (game.map->can_walk(owner.posX, owner.posY + stepdy))
		{
			owner.posY += stepdy;
		}
	}
		else
		{
			std::cout << "Error: game.map is null" << std::endl;
			exit(-1);
		}

	}

	else if (owner.attacker)
	{
		owner.attacker->attack(owner, *game.player);
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
	QUIT = 'q',

	// switch player1 keypad F1
	SWITCH_P1 = 'v',

	// switch player2
	SWITCH_P2 = 'b',

	// switch player3
	SWITCH_P3 = 'n',

	// switch all
	SWITCH_ALL = 'm'
};

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



bool PlayerAi::levelUpUpdate(Actor& owner)
{
	// level up if needed
	int levelUpXp = getNextLevelXp();
	if (owner.destructible != nullptr)
	{
	if (owner.destructible->xp >= levelUpXp)
	{
		xpLevel++;
		owner.destructible->xp -= levelUpXp;
		game.gui->log_message(WHITE_PAIR, "Your battle skills grow stronger! You reached level %d", xpLevel);
		game.dispay_stats(xpLevel);
	}
	}
	else
	{
		std::cout << "Error: PlayerAi::levelUpUpdate(Actor& owner). owner.destructible is null" << std::endl;
		exit(-1);
	}

	if (owner.destructible && owner.destructible->is_dead())
	{
		return true;
	}
	return false;
}

void PlayerAi::update(Actor& owner)
{
	std::clog << "PlayerAi::update(Actor& owner)" << std::endl;
	levelUpUpdate(owner);

	int dx = 0, dy = 0; // movement delta

	// TODO : check if this clear() need to be relocated
	/*clear();*/ // this is here for clearing the level up screen 
	// the controls for the player movement
	switch (static_cast<Controls>(game.keyPress))
	{
	case Controls::UP:
	case Controls::UP_ARROW:
	case Controls::UP_ARROW_NUMPAD:
	{
		dy = -1;
		break;
	}

	case Controls::DOWN:
	case Controls::DOWN_ARROW:
	case Controls::DOWN_ARROW_NUMPAD:
	{
		dy = 1;
		break;
	}

	case Controls::LEFT:
	case Controls::LEFT_ARROW:
	case Controls::LEFT_ARROW_NUMPAD:
	{
		dx = -1;
		break;
	}

	case Controls::RIGHT:
	case Controls::RIGHT_ARROW:
	case Controls::RIGHT_ARROW_NUMPAD:
	{
		dx = 1;
		break;
	}

	case Controls::UP_LEFT:
	case Controls::UP_LEFT_ARROW_NUMPAD:
	{
		dx = -1;
		dy = -1;
		break;
	}

	case Controls::UP_RIGHT:
	case Controls::UP_RIGHT_ARROW_NUMPAD:
	{
		dx = 1;
		dy = -1;
		break;
	}

	case Controls::DOWN_LEFT:
	case Controls::DOWN_LEFT_ARROW_NUMPAD:
	{
		dx = -1;
		dy = 1;
		break;
	}

	case Controls::DOWN_RIGHT:
	case Controls::DOWN_RIGHT_ARROW_NUMPAD:
	{
		dx = 1;
		dy = 1;
		break;
	}

	case Controls::WAIT:
	case Controls::WAIT_ARROW_NUMPAD:
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		break;
	}

	case Controls::HIT_SELF:
	{
		if (game.player)
		{
			if (game.player->attacker)
			{
		game.player->attacker->attack(*game.player, *game.player);
			}
			else
			{
				std::cout << "Error: PlayerAi::update(Actor& owner). game.player->attacker is null" << std::endl;
				exit(-1);
			}

		}
		else
		{
			std::cout << "Error: PlayerAi::update(Actor& owner). game.player is null" << std::endl;
			exit(-1);
		}

		break;
	}

	case Controls::MOUSE:
	{
		std::cout << "mouse" << std::endl;
		request_mouse_pos();
		break;
	}

	case Controls::PICK:
	case Controls::PICK_SHIFT_STAR:
	case Controls::PICK_NUMPAD:
	{
		pick_item(owner);
		break;
	}

	case Controls::DROP:
	{
		std::shared_ptr<Actor> actor = choseFromInventory(owner, 'a');
		if (actor) 
		{
			actor->pickable->drop(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	break;
	}

	case Controls::INVENTORY:
	{
		/*handleActionKey(owner, game.keyPress);*/
		display_inventory(owner);
		break;
	}

	case Controls::QUIT:
		{
		game.run = false;
			mvprintw(29, 0, "You quit the game ! Press any key ...");
		break;
	}

	case Controls::ESCAPE: // if escape key is pressed bring the game menu
	{
		Menu menu;
		menu.menu();
		break;
	}

	case Controls::DESCEND:
	{
		if (game.stairs->posX == owner.posX && game.stairs->posY == owner.posY)
		{
			game.next_level();
		}
		break;
	}

	case Controls::TARGET:
	{
		game.target();
		break;
	}

	case Controls::CHAR_SHEET:
	{
		game.display_character_sheet();
		break;
	}

	default:break;
	}

	// compute FOV if needed
	if (dx != 0 || dy != 0)
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		if (moveOrAttack(owner, owner.posX + dx, owner.posY + dy))
		{
			game.map->compute_fov();
		}
	}
	std::clog << "PlayerAi::update(Actor& owner) end" << std::endl;
}

void PlayerAi::pick_item(Actor& owner)
{
	std::clog << "You try to pick something..." << std::endl;

	bool found = false;
	for (const auto actor : game.actors)
	{
		/*std::clog << "Checking actor " << actor->name << std::endl;*/

		if (
			actor->pickable
			&&
			actor->posX == owner.posX
			&&
			actor->posY == owner.posY
			)
		{
			if (actor->pickable->pick(*actor, owner))
			{
				found = true;
				game.gui->log_message(DARK_GROUND_PAIR, "You take the %s.", actor->name.c_str());
				break;
			}
			else if (!found)
			{
				found = true;
				game.gui->log_message(HPBARMISSING_PAIR, "Your inventory is full.");
			}
		}
	}

	if (!found)
	{
		game.gui->log_message(HPBARFULL_PAIR, "There is nothing to pick up.");
	}

	game.gameStatus = Game::GameStatus::NEW_TURN;
}

void PlayerAi::display_inventory(Actor& owner)
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
	
	for (auto& actor : owner.container->inventoryList)
	{
		/*game.gui->log_message(y, 0, "(%c) %s", shortcut, actor->name);*/

		mvwprintw(inv, y, 1, "(%c) %s", shortcut, actor->name.c_str());
		y++;
		shortcut++;
	}

	/*// blit the inventory console on the root console
	TCODConsole::blit(&con, 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT,
		TCODConsole::root, game.screenWidth / 2 - INVENTORY_WIDTH / 2,
		game.screenHeight / 2 - INVENTORY_HEIGHT / 2);
	TCODConsole::flush();*/
	wrefresh(inv);

	//std::clog << "You display inventory" << std::endl;
	//if (actor)
	//{
	//	actor->pickable->use(actor, owner);
	//}

	Gui gui; // create a new Gui object for the inventory menu to use for rendering the inventory
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
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'b':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'c':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'd':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}
	clear();
	break;

	case 'e':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}

	case 'f':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}

	case 'h':
	{
		// use item
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}

	default:
		clear();
		break;
	}
}

std::shared_ptr<Actor> PlayerAi::choseFromInventory(Actor& owner, int ascii)
{
	std::clog << "You chose from inventory" << std::endl;
	if (ascii == 'a')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[0];
		}
	}
	if (ascii == 'b')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[1];
		}
	}
	if (ascii == 'c')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[2];
		}
	}
	if (ascii == 'd')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[3];
		}
	}
	if (ascii == 'e')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[4];
		}
	}
	if (ascii == 'f')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[5];
		}
	}
	if (ascii == 'g')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[6];
		}
	}
	// if case is 'h'
	if (ascii == 'h')
	{
		if (owner.container->inventoryList.size() > 0)
		{
			return owner.container->inventoryList[7];
		}
	}

	return nullptr;
}

void PlayerAi::load(TCODZip& zip)
{
	// this is a player, so nothing to load
	// because the player is always the same ? 
	// or is this for loading a saved game ?
	// if so, then the player is not always the same
	// so this is not correct
	// but I don't know how to do it yet
}

void PlayerAi::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::PLAYER));
}

bool PlayerAi::moveOrAttack(Actor& owner, int targetx, int targety)
{
	//if (game.map != nullptr)
	//{
	//	if (game.map->is_wall(targetx, targety))
	//	{
	//		return false;
	//	}
	//}
	//else
	//{
	//	std::clog << "map is null in moveOrAttack" << std::endl;
	//	exit(-1);
	//}
	if (game.map != nullptr)
	{
	if (game.map->is_wall(targety, targetx))
	{
		return false;
	}
	}
	else
	{
		std::cout << "Error: moveOrAttack() called on actor with no map." << std::endl;
		exit(-1);
	}

	// look for living actors to attack
	for (const auto& actor : game.actors)
	{
		const auto isNotDead = (actor->destructible && !actor->destructible->is_dead());
		if (isNotDead && actor->posX == targetx && actor->posY == targety)
		{
			owner.attacker->attack(owner, *actor);
			return false;
		}
	}
		else
		{
			std::cout << "Error: moveOrAttack() called on actor with null actor." << std::endl;
			exit(-1);
		}
	}

	// look for corpses or items
	for (const auto& actor : game.actors)
	{
		const auto isDeadCorpseOrItem = (actor->destructible && actor->destructible->is_dead()) || actor->pickable;

		if (isDeadCorpseOrItem && actor->posX == targetx && actor->posY == targety)
		{
			mvprintw(29, 0, "There's a %s here\n", actor->name.c_str());
		}
	}

	owner.posX = targetx;
	owner.posY = targety;

	return true;
}

//==ConfusedMonsterAi==
ConfusedMonsterAi::ConfusedMonsterAi(int nbTurns, std::shared_ptr<Ai> oldAi) :
	nbTurns(nbTurns),
	oldAi(oldAi)
{
}

void ConfusedMonsterAi::update(Actor& owner)
{
	gsl::not_null<TCODRandom*> rng = TCODRandom::getInstance();
	int dx = rng->getInt(-1, 1);
	int dy = rng->getInt(-1, 1);

	if (dx != 0 || dy != 0)
	{
		int destx = owner.posX + dx;
		int desty = owner.posY + dy;

		if (game.map != nullptr)
		{
		if (game.map->can_walk(desty, destx))
		{
			owner.posX = destx;
			owner.posY = desty;
		}
		else
		{
			std::shared_ptr<Actor> actor = game.get_actor(destx, desty);
			if (actor)
			{
				owner.attacker->attack(owner, *actor);
			}
			}
		}
		else
		{
			std::cout << "Error: update() called on actor with no map." << std::endl;
			exit(-1);
		}
	}

	nbTurns--;
	if (nbTurns == 0)
	{
		owner.ai = oldAi;
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
	if (oldAi != nullptr)
	{
	oldAi->save(zip);
}
	else
	{
		std::cout << "Error: save() called on actor with no oldAi." << std::endl;
		exit(-1);
	}
}
//====

// end of file: Ai.cpp
