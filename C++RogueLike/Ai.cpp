#include <iostream>

#include "main.h"
#include "Colors.h"

//====
//a constexpr for tracking the number of turns
constexpr auto TRACKING_TURNS = 3;

//====
MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor* owner)
{

	if (owner->destructible && owner->destructible->isDead()) // if the owner is dead
	{
		return; // do nothing
	}

	if (engine.map->isInFov(owner->x, owner->y)) // if the owner is in the fov
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
		moveOrAttack(owner, engine.player->x, engine.player->y); // move or attack the player
	}
}

void MonsterAi::moveOrAttack(Actor* owner, int targetx, int targety)
{
	int dx = targetx - owner->x; // get the x distance
	int dy = targety - owner->y; // get the y distance
	int stepdx = (dx > 0 ? 1 : -1); // get the x step
	int stepdy = (dy > 0 ? 1 : -1); // get the y step
	double distance = sqrt(dx * dx + dy * dy); // get the distance
	if (distance >= 2)
	{
		dx = (int)(round(dx / distance));
		dy = (int)(round(dy / distance));
		if (engine.map->canWalk(owner->x + dx, owner->y + dy))
		{
			owner->x += dx;
			owner->y += dy;
		}
		else if (engine.map->canWalk(owner->x + stepdx, owner->y))
		{
			owner->x += stepdx;
		}
		else if (engine.map->canWalk(owner->x, owner->y + stepdy))
		{
			owner->y += stepdy;
		}
	}
	else if (owner->attacker)
	{
		owner->attacker->attack(owner, engine.player);
	}
}


//====
// how many turns the monster chases the player
// after losing his sight

void PlayerAi::update(Actor* owner)
{
	if (owner->destructible && owner->destructible->isDead())
	{
		return;
	}
	
	int dx = 0, dy = 0;
	int key = getch();
	clear();
	switch (key)
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
		engine.gameStatus = Engine::NEW_TURN;
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
		handleActionKey(owner, key);
		break;

	// detect the key press and pass it to the handleActionKey function
	case 'i':
		handleActionKey(owner, key);
		break;
	
	// if 'p' is pressed pick health potion
	case 'p':
		engine.player->pickItem(engine.player->x, engine.player->y);
		break;
	
	case QUIT:
		exit(0);
		break;
	}

	if (dx!=0||dy!=0)
	{
		engine.gameStatus = Engine::NEW_TURN;
		if (moveOrAttack(owner,owner->x+dx,owner->y+dy))
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
				actor->x == owner->x
				&&
				actor->y == owner->y
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
	engine.gameStatus = Engine::NEW_TURN;
	} // end of case 'g'
	break;

	case 'i': // display inventory
	{
		std::clog << "You display inventory" << std::endl;
		Actor* actor = choseFromInventory(owner);
		if (actor) {
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::NEW_TURN;
		}
	}
	break;
	
	} // end of switch statement
}

Actor* PlayerAi::choseFromInventory(Actor* owner)
{
	constexpr int INVENTORY_WIDTH = 50;
	constexpr int INVENTORY_HEIGHT = 28;
	const int center_y = getmaxy(stdscr) / 2;
	const int center_x = getmaxx(stdscr) / 2;
	
	refresh();

	// Note that thanks to the STATIC keyword,
	// the console is only created the first time.
	static TCODConsole con(INVENTORY_WIDTH, INVENTORY_HEIGHT);
	WINDOW* inv = newwin(
		INVENTORY_HEIGHT, // int nlines
		INVENTORY_WIDTH, // int ncols
		center_y, // int begy
		center_x // int begx
	);

	box(inv, 0, 0);
	
	/*// display the inventory frame*/
	/*con.setDefaultForeground(TCODColor(200, 180, 50));*/
	con.printFrame(0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT, true, TCOD_BKGND_DEFAULT, "inventory");
	
	/*// display the items with their keyboard shortcut*/
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
		engine.gui->log_message(y, 0, "(%c) %s", shortcut, actor->name);
		y++;
		shortcut++;
	}
	
	/*// blit the inventory console on the root console
	TCODConsole::blit(&con, 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT,
		TCODConsole::root, engine.screenWidth / 2 - INVENTORY_WIDTH / 2,
		engine.screenHeight / 2 - INVENTORY_HEIGHT / 2);
	TCODConsole::flush();*/
	
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
	wrefresh(inv);
	return NULL;
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
			actor->x == targetx
			&&
			actor->y == targety
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
			actor->x == targetx
			&&
			actor->y == targety
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

		owner->x = targetx;
		owner->y = targety;

		return true;
}

//====