// file: Ai.cpp
#include <iostream>
#include <curses.h>
#include <gsl/util>
#include <gsl/pointers>

#include "main.h"
#include "Menu.h"
#include "Colors.h"
#include "AiMonster.h"

//==AI==
std::shared_ptr<Ai> Ai::create(TCODZip& zip) 
{
	const AiType type = gsl::narrow_cast<AiType>(zip.getInt());
	std::shared_ptr<Ai> ai = nullptr;

	switch (type) 
	{

	case AiType::PLAYER:
	{
		ai = std::make_shared<PlayerAi>();
		break;
	}

	case AiType::MONSTER:
	{
		ai = std::make_shared<AiMonster>();
		break;
	}
	
	case AiType::CONFUSED_MONSTER:
	{
		ai = std::make_shared<ConfusedMonsterAi>(0, nullptr);
		break;
	}

	}

	if (ai != nullptr)
	{
		ai->load(zip);
	}
	else
	{
		std::cout << "Error: Ai::create() - ai is null" << std::endl;
		exit(-1);
	}

	return ai; // TODO: don't return nullptr
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

constexpr int LEVEL_UP_BASE = 200;
constexpr int LEVEL_UP_FACTOR = 150;

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

	if (game.actors.empty())
	{
		std::cout << "Error: PlayerAi::pick_item(Actor& owner). game.actors is empty" << std::endl;
		exit(-1);
	}

	bool found = false;

	for (const auto& actor : game.actors)
	{
		// Skip null actors
		if (actor == nullptr)
		{
			std::cout << "Error: PlayerAi::pick_item(Actor& owner). game.actors contains null pointer" << std::endl;
			exit(-1);
		}

		std::clog << "Checking actor " << actor->name << std::endl;
		std::cout << "Checking actor " << actor->name << std::endl;

		// Skip actors without a pickable component
		if (actor->pickable == nullptr)
		{
			std::clog << "Skipping actor " << actor->name << " because it has no pickable component" << std::endl;
			std::cout << "Skipping actor " << actor->name << " because it has no pickable component" << std::endl;
			continue;
		}

		// Check if the actor can be picked up at the player's position
		if (is_pickable_at_position(*actor, owner))
		{
			// Try to pick up the actor
			found = try_pick_actor(*actor, owner);

			if (found)
			{
				break;
			}
		}
	}

	// Log a message if there's nothing to pick up
	if (!found)
	{
		game.gui->log_message(HPBARFULL_PAIR, "There is nothing to pick up.");
	}

	game.gameStatus = Game::GameStatus::NEW_TURN;
}

bool PlayerAi::is_pickable_at_position(const Actor& actor, const Actor& owner) const
{
	return actor.posX == owner.posX && actor.posY == owner.posY;
}

bool PlayerAi::try_pick_actor(Actor& actor, Actor& owner)
{
	std::clog << "Trying to pick actor " << actor.name << std::endl;
	std::cout << "Trying to pick actor " << actor.name << std::endl;

	bool picked = actor.pickable->pick(actor, owner);

	if (picked)
	{
		if (game.gui != nullptr)
		{
			game.gui->log_message(DARK_GROUND_PAIR, "You take the %s.", actor.name.c_str());
		}
		else
		{
			std::cout << "Error: PlayerAi::try_pick_actor(Actor& actor, Actor& owner). game.gui is null pointer" << std::endl;
			exit(-1);
		}
	}
	else
	{
		if (game.gui != nullptr)
		{
			game.gui->log_message(HPBARMISSING_PAIR, "Your inventory is full.");
		}
		else
		{
			std::cout << "Error: PlayerAi::try_pick_actor(Actor& actor, Actor& owner). game.gui is null pointer" << std::endl;
			exit(-1);
		}
	}

	return picked;
}

constexpr int INVENTORY_HEIGHT = 29;
constexpr int INVENTORY_WIDTH = 30;

void PlayerAi::displayInventoryItems(WINDOW* inv, Actor& owner)
{
	int shortcut = 'a';
	int y = 1;
	for (const auto& actor : owner.container->inventoryList)
	{
		if (actor != nullptr)
		{
			mvwprintw(inv, y, 1, "(%c) %s", shortcut, actor->name.c_str());
		}
		y++;
		shortcut++;
	}
}

void PlayerAi::display_inventory(Actor& owner)
{
	refresh();

	WINDOW* inv = newwin(INVENTORY_HEIGHT, INVENTORY_WIDTH, 0, 0);

	box(inv, 0, 0);
	mvwprintw(inv, 0, 0, "Inventory");

	if (owner.container->inventoryList.size() > 0)
	{
		displayInventoryItems(inv, owner);
	}
	else
	{
		int y = 1;
		mvwprintw(inv, y, 1, "Your inventory is empty.");
	}

	wrefresh(inv);

	int inventoryInput = getch();
	if (inventoryInput == static_cast<int>(Controls::ESCAPE))
	{
		delwin(inv);
		clear();
	}
	else if (inventoryInput >= 'a' && inventoryInput <= 'z')
	{
		std::shared_ptr<Actor> actor = choseFromInventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
		delwin(inv);
		clear();
	}
	else
	{
		delwin(inv);
		clear();
	}
}

std::shared_ptr<Actor> PlayerAi::choseFromInventory(Actor& owner, int ascii)
{
	std::clog << "You chose from inventory" << std::endl;
	if (owner.container != nullptr)
	{
		if (ascii == 'a')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				/*return owner.container->inventoryList[0];*/
				// prefer to use gsl::at() instead pf unchecked subscript operator (bounds.4).
				// gsl::at() will throw an exception if the index is out of bounds
				return gsl::at(owner.container->inventoryList, 0);
			}
		}
		if (ascii == 'b')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 1);
			}
		}
		if (ascii == 'c')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 2);
			}
		}
		if (ascii == 'd')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 3);
			}
		}
		if (ascii == 'e')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 4);
			}
		}
		if (ascii == 'f')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 5);
			}
		}
		if (ascii == 'g')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 6);
			}
		}
		// if case is 'h'
		if (ascii == 'h')
		{
			if (owner.container->inventoryList.size() > 0)
			{
				return gsl::at(owner.container->inventoryList, 7);
			}
		}
	}
	else
	{
		std::cout << "Error: choseFromInventory() called on actor with no container." << std::endl;
		exit(-1);
	}

	return nullptr; // TODO: don't hand out nullptrs
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

bool is_not_dead(const Actor& actor)
{
	if (actor.destructible && !actor.destructible->is_dead())
	{
		return true;
	}
	else
	{
		return false;
	}
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
		if (actor != nullptr)
		{
			if (is_not_dead(*actor) && actor->posX == targetx && actor->posY == targety)
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
