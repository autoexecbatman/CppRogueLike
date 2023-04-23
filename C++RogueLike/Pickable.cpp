// file: Pickable.cpp
#include <gsl/util>
#include <vector>

#include "main.h"
#include "Colors.h"

//==PICKABLE==
bool Pickable::pick(Actor& owner, const Actor& wearer)
{
	if (wearer.container && wearer.container->add(owner))
	{
		// TODO : Fix this to remove the item from the map after it has been picked up.
		/*game.actors.erase(
		std::remove(game.actors.begin(), game.actors.end(), owner),
		game.actors.end()
		);*/

		game.gui->log_message( // displays the message
			COLOR_WHITE,
			"%s picks up a %s.",
			wearer.name.c_str(),
			owner.name.c_str()
		);
		
		// first iterate through the list of actors
		for (auto actor : game.actors)
		{
			// if the actor is the owner of the item
			if (actor.get() == &owner)
			{
				// remove the item from the list of actors
				//game.actors.erase(
				//	std::remove(game.actors.begin(), game.actors.end(), actor),
				//	game.actors.end()
				//);
				
				std::erase(game.actors, actor);

				//actor->ch = '.';
				//actor->col = WHITE_PAIR;

				break;
			}
		} 

		return true;
	}

	return false;
}

void Pickable::drop(Actor& owner, Actor& wearer)
{
	//if (wearer->container)
	//{
	//	wearer->container->remove(owner);
	//	owner->posX = wearer->posX;
	//	owner->posY = wearer->posY;
	//	owner->ch = wearer->ch;
	//	owner->col = wearer->col;
	//	engine.actors.emplace_back(owner);
	//	engine.send_to_back(owner);
	//}
}

bool Pickable::use(Actor& owner, Actor& wearer)
{
	std::clog << "Using item" << std::endl;
	if (wearer.container)
	{
		wearer.container->remove(owner);
		/*delete owner;*/
		return true;
	}
	return false;
}

std::shared_ptr<Pickable> Pickable::create(TCODZip& zip)
{
	PickableType type = (PickableType)zip.getInt();
	std::shared_ptr<Pickable> pickable = nullptr;

	switch (type)
	{

	case PickableType::HEALER:
	{
		/*pickable = new Healer(0);*/
		pickable = std::make_shared<Healer>(0);
		break;
	}

	case PickableType::LIGHTNING_BOLT:
	{
		/*pickable = new LightningBolt(0, 0);*/
		pickable = std::make_shared<LightningBolt>(0, 0);
		break;
	}

	case PickableType::CONFUSER:
	{
		/*pickable = new Confuser(0, 0);*/
		pickable = std::make_shared<Confuser>(0, 0);
		break;
	}

	case PickableType::FIREBALL:
	{
		/*pickable = new Fireball(0, 0);*/
		pickable = std::make_shared<Fireball>(0, 0);
		break;
	}

	}

	pickable->load(zip);

	return pickable;
}

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Actor& owner, Actor& wearer)
{
	if (wearer.destructible)
	{
		int amountHealed = wearer.destructible->heal(amountToHeal);

		if (amountHealed > 0)
		{
			return Pickable::use(owner, wearer);
		}
	}

	return false;
}

void Healer::load(TCODZip& zip) 
{
	amountToHeal = zip.getInt();
}

void Healer::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::HEALER));
	zip.putInt(amountToHeal);
}

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(int maxRange, int damage) : maxRange(maxRange), damage(damage)
{
}

bool LightningBolt::use(Actor& owner, Actor& wearer)
{
	// find closest enemy (inside a maximum range)
	std::shared_ptr<Actor> closestMonster = game.get_closest_monster(wearer.posX, wearer.posY, maxRange);

	if (!closestMonster)
	{
		game.gui->log_message(HPBARMISSING_PAIR, "No enemy is close enough to strike.");

		return false;
	}

	// hit closest monster for <damage> hit points
	game.gui->log_message(HPBARFULL_PAIR, "A lighting bolt strikes the %s with a loud thunder!\n"
		"The damage is %g hit points.", closestMonster->name, damage);
	closestMonster->destructible->take_damage(*closestMonster, damage);

	return Pickable::use(owner, wearer);
}

void LightningBolt::load(TCODZip& zip) 
{
	maxRange = zip.getInt();
	damage = zip.getInt();
}

void LightningBolt::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::LIGHTNING_BOLT));
	zip.putInt(maxRange);
	zip.putInt(damage);
}

//==Fireball==
Fireball::Fireball(int range, int damage) : LightningBolt(range, damage) {}

bool Fireball::use(Actor& owner, Actor& wearer)
{
	game.gui->log_message(
		WHITE_PAIR,
		"Left-click a target tile for the fireball,\nor right-click to cancel."
	);

	int x{ 0 };
	int y{ 0 };

	if (!game.pick_tile(&x, &y , Fireball::maxRange)) // <-- runs a while loop here
	{
		return false;
	}

	// burn everything in <range> (including player)
	game.gui->log_message(
		WHITE_PAIR,
		"The fireball explodes, burning everything within %d tiles!",
		Fireball::maxRange
	);

	// make impact explosion using a circle algorithm and curses library 
	// (this is a bit of a hack, but it works)
	
	// get the center of the explosion
	int centerX = x;
	int centerY = y;

	// get the radius of the explosion
	/*int radius = static_cast<int>(Fireball::maxRange);*/
	// use gsl
	int radius = gsl::narrow_cast<int>(Fireball::maxRange);

	int sideLength = radius * 2 + 1;

	// calculate the chebyshev distance from the player to maxRange
	int chebyshev = std::max(abs(centerX - (centerX - radius)), abs(centerY - (centerY - radius)));

	int height = sideLength;
	int width = sideLength;

	int centerOfExplosionY = centerY - chebyshev;
	int centerOfExplosionX = centerX - chebyshev;

	WINDOW* explosionWindow = newwin(
		height, // number of rows
		width, // number of columns
		centerOfExplosionY, // y position
		centerOfExplosionX // x position
	);

	// draw fire inside the window
	wbkgd(explosionWindow, COLOR_PAIR(FIREBALL_PAIR));

	// add a character 'x' to the center of the explosion
	// add a random color to x

	bool run = true;

	nodelay(explosionWindow, true);

	// Reduce the number of iterations from 1000 to 50
	for (int numLoops = 0; numLoops < 50; numLoops++) {
		// Draw the explosion
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int randColor = rand() % 7 + 1;
				mvwaddch(explosionWindow, j, i, 'x' | COLOR_PAIR(randColor));
			}
		}
		wrefresh(explosionWindow);
		napms(20);  // Add a small delay between each frame
	}

	nodelay(explosionWindow, false);
	wrefresh(explosionWindow);
	napms(1000);  // Keep the final frame for 1 second

	delwin(explosionWindow);

	for (const auto& actor : game.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->is_dead()
			&&
			actor->get_distance(x, y) <= Fireball::maxRange
			)
		{
			game.gui->log_message(WHITE_PAIR, "The %s gets burned!\nfor %d hp.", actor->name.c_str(), damage);
			animation(actor->posX, actor->posY, maxRange);
		}
	}

	for (const auto& actor : game.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->is_dead()
			&&
			actor->get_distance(x, y) <= Fireball::maxRange
			)
		{
			actor->destructible->take_damage(*actor, damage);
		}
	}

	return Pickable::use(owner, wearer);
}


void Fireball::animation(int x, int y , int maxRange)
{
	bool run = true;
	while (run == true)
	{
		clear();
		game.render();

		attron(COLOR_PAIR(FIREBALL_PAIR));
		mvprintw(y, x, "~");
		attroff(COLOR_PAIR(FIREBALL_PAIR));
		
		// ask the player to press a key to continue
		mvprintw(29, 0, "press 'SPACE' to continue");
		refresh();
		
		if (getch() == ' ')
		{
			run = false;
		}
	}
}

void Fireball::load(TCODZip& zip)
{
	maxRange = zip.getInt();
	damage = zip.getInt();
}

void Fireball::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::FIREBALL));
	zip.putInt(maxRange);
	zip.putInt(damage);
}

//==Confuser==
Confuser::Confuser(int nbTurns, int range) : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor& owner, Actor& wearer)
{
	game.gui->log_message(WHITE_PAIR, "Left-click an enemy to confuse it,\nor right-click to cancel.");

	int x, y;

	if (!game.pick_tile(&x, &y, range))
	{
		return false;
	}

	auto actor = game.get_actor(x, y);

	if (!actor)
	{
		return false;
	}

	// replace the monster's AI with a confused one; 
	// after <nbTurns> turns the old AI will be restored
	/*ConfusedMonsterAi* confusedAi = new ConfusedMonsterAi(nbTurns, actor->ai);*/
	auto confusedAi = std::make_shared<ConfusedMonsterAi>(nbTurns, actor->ai);
	actor->ai = confusedAi;
	game.gui->log_message(WHITE_PAIR, "The eyes of the %s look vacant,\nas he starts to stumble around!", actor->name);
	

	return Pickable::use(owner, wearer);
}

void Confuser::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	range = zip.getInt();
}

void Confuser::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::CONFUSER));
	zip.putInt(nbTurns);
	zip.putInt(range);
}

// end of file: Pickable.cpp
