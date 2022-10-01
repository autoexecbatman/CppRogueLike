#include <iostream>

#include "main.h"
#include "Colors.h"

//====
//a constexpr for tracking the number of turns
constexpr auto TRACKING_TURNS = 3;

//==CONTROLS==
// the enumeration for the controls of the player
// TODO : convert to enum class if needed.
enum class CONTROLS : char
{
	UP = 'w',
	DOWN = 's',
	LEFT = 'a',
	RIGHT = 'd',
	QUIT = 'q'
};

//==AI==
Ai* Ai::create(TCODZip& zip) 
{
	AiType type = (AiType)zip.getInt();
	Ai* ai = nullptr;
	switch (type) {
	case PLAYER: ai = new PlayerAi(); break;
	case MONSTER: ai = new MonsterAi(); break;
	}
	ai->load(zip);
	return ai;
}

//==MONSTER_AI==
MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor* owner)
{

	if (owner->destructible && owner->destructible->isDead()) // if the owner is dead
	{
		return; // do nothing
	}

	if (engine.map->isInFov(owner->posX, owner->posY)) // if the owner is in the fov
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
	zip.putInt(MONSTER);
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
		dx = (int)(round(dx / distance));
		dy = (int)(round(dy / distance));
		if (engine.map->canWalk(owner->posX + dx, owner->posY + dy))
		{
			owner->posX += dx;
			owner->posY += dy;
		}
		else if (engine.map->canWalk(owner->posX + stepdx, owner->posY))
		{
			owner->posX += stepdx;
		}
		else if (engine.map->canWalk(owner->posX, owner->posY + stepdy))
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

PlayerAi::PlayerAi() : xpLevel(1) {}

const int LEVEL_UP_BASE = 200;
const int LEVEL_UP_FACTOR = 150;

int PlayerAi::getNextLevelXp()
{
	return LEVEL_UP_BASE + xpLevel * LEVEL_UP_FACTOR;
}

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

	if (owner->destructible && owner->destructible->isDead())
	{
		return;
	}
	
	int dx = 0, dy = 0;
	
	clear();
	switch (engine.keyPress)
	{
	case '8':
		dy = -1;
		break;
	case '2':
		dy = 1;
		break;
	case '4':
		dx = -1;
		break;
	case '6':
		dx = 1;
		break;
	// if numpad key 7 is pressed move diagonaly up left
	case '7':
		dx = -1;
		dy = -1;
		break;
		// if numpad key 9 is pressed move diagonaly up right
	case '9':
		dx = 1;
		dy = -1;
		break;
	// if numpad key 1 is pressed move diagonaly down left
	case '1':
		dx = -1;
		dy = 1;
		break;
	// if numpad key 3 is pressed move diagonaly down right
	case '3':
		dx = 1;
		dy = 1;
		break;
	// numpad key 5 is pressed a turn will pass
	case '5':
		engine.gameStatus = Engine::GameStatus::NEW_TURN;
		break;
	case ' ': 
		engine.player->attacker->attack(engine.player, engine.player);
		break;
	case KEY_MOUSE:
		std::cout << "mouse" << std::endl;
		request_mouse_pos();
		break;
	// detect the key press and pass it to the handleActionKey function
	case 'g':
		handleActionKey(owner, engine.keyPress);
		break;

	// detect the key press and pass it to the handleActionKey function
	case 'i':
		handleActionKey(owner, engine.keyPress);
		break;
	
	// use the health potion
	case 'a':
		handleActionKey(owner, engine.keyPress);
		break;

	// if 'p' is pressed pick health potion
	case 'p':
		engine.player->pickItem(engine.player->posX, engine.player->posY);
		break;
	
	case static_cast<int>(CONTROLS::QUIT):
		engine.run = false;
		if (engine.run == false)
		{
			mvprintw(29, 0, "You quit the game ! Press any key ...");
		}
		break;

	// if escape key is pressed bring the game menu
	case 27:
		engine.game_menu();
		break;

	case '>':
		if (engine.stairs->posX == owner->posX && engine.stairs->posY == owner->posY)
		{
			engine.nextLevel();
		}
		break;
	
	default:break;
	}

	if (dx!=0||dy!=0)
	{
		engine.gameStatus = Engine::GameStatus::NEW_TURN;
		if (moveOrAttack(owner,owner->posX+dx,owner->posY+dy))
		{
			engine.map->computeFov();
		}
	}
}

void PlayerAi::handleActionKey(Actor* owner, int ascii)
{
	std::clog << "handleActionKey" << std::endl;
	switch (ascii)
	{
	case 'g':
	{
		std::clog << "You pick" << std::endl;
		bool found = false;
		for (const auto actor : engine.actors)
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
	} // end of case 'g'
	break;

	case 'i': // display inventory
	{
		Actor* actor = choseFromInventory(owner, ascii);
		std::clog << "You display inventory" << std::endl;
		if (actor) 
		{
			actor->pickable->use(actor, owner);
		}
	}
	break;

	case 'a': // use the potion at the same time
	{
		Actor* actor = choseFromInventory(owner, ascii);
		std::clog << "You heal" << std::endl;
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::GameStatus::NEW_TURN;
		}
	}
	break;
	
	} // end of switch statement
}

void PlayerAi::load(TCODZip& zip)
{
}

void PlayerAi::save(TCODZip& zip)
{
	zip.putInt(PLAYER);
}

Actor* PlayerAi::choseFromInventory(Actor* owner, int ascii)
{
	constexpr int INVENTORY_HEIGHT = 7;
	constexpr int INVENTORY_WIDTH = 30;
	
	
	// Note that thanks to the STATIC keyword,
	// the console is only created the first time.
	/*static TCODConsole con(INVENTORY_WIDTH, INVENTORY_HEIGHT);*/
	if (ascii == 'i')
	{
		refresh();

		WINDOW* inv = newwin(
			INVENTORY_HEIGHT, // int nlines
			INVENTORY_WIDTH, // int ncols
			22, // int begy
			30 // int begx
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

		for (Actor* actor : owner->container->inventory)
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
	}
	/*// wait for a key press
	TCOD_key_t key;
	TCODSystem::waitForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL, true);*/
	
	/*if (key.vk == TCODK_CHAR) {
		int actorIndex = key.c - 'a';
		if (actorIndex >= 0 && actorIndex < owner->container->inventory.size()) {
			return owner->container->inventory.get(actorIndex);
		}
		return NULL;
	}*/

	/*int key = getch();*/
	if (ascii == 'a') // <<-- ??? get access for keypress
	{
		// when 'a' is pressed
		// find and use potion
		for (Actor* actor : owner->container->inventory)
		{
			if (!strcmp(actor->name, "health potion"))
			{
				std::cout << " if is true ! " << std::endl;
				return actor;
			}
		}
	}
	return nullptr;
}

bool PlayerAi::moveOrAttack(Actor* owner, int targetx, int targety) 
{
	if (engine.map->isWall(targety, targetx))
	{
		return false;
	}

	// look for living actors to attack
	for (const auto& actor : engine.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->isDead()
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
			actor->destructible->isDead()
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

//====