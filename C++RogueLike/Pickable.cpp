#include <vector>
#include "main.h"
#include "Colors.h"

//==PICKABLE==
bool Pickable::pick(Actor* owner, Actor* wearer)
{
	if (wearer->container && wearer->container->add(owner))
	{
		engine.actors.erase(std::remove(engine.actors.begin(), engine.actors.end(), owner), engine.actors.end());
		return true;
	}
	return false;
}

void Pickable::drop(Actor* owner, Actor* wearer)
{
	if (wearer->container)
	{
		wearer->container->remove(owner);
		owner->posX = wearer->posX;
		owner->posY = wearer->posY;
		owner->ch = wearer->ch;
		owner->col = wearer->col;
		engine.actors.push_back(owner);
		engine.send_to_back(owner);
	}
}

bool Pickable::use(Actor* owner, Actor* wearer)
{
	if (wearer->container)
	{
		wearer->container->remove(owner);
		delete owner;
		return true;
	}
	return false;
}

Pickable* Pickable::create(TCODZip& zip) 
{
	PickableType type = (PickableType)zip.getInt();
	Pickable* pickable = nullptr;

	switch (type)
	{

	case PickableType::HEALER:
	{
		pickable = new Healer(0);
		break;
	}

	case PickableType::LIGHTNING_BOLT:
	{
		pickable = new LightningBolt(0, 0);
		break;
	}

	case PickableType::CONFUSER:
	{
		pickable = new Confuser(0, 0);
		break;
	}

	case PickableType::FIREBALL:
	{
		pickable = new Fireball(0, 0);
		break;
	}

	}

	pickable->load(zip);

	return pickable;
}

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Actor* owner, Actor* wearer)
{
	if (wearer->destructible)
	{
		int amountHealed = wearer->destructible->heal(amountToHeal);

		if (amountHealed > 0)
		{
			return Pickable::use(owner, wearer);
		}
	}

	return false;
}

void Healer::load(TCODZip& zip) 
{
	amountToHeal = zip.getFloat();
}

void Healer::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::HEALER));
	zip.putFloat(amountToHeal);
}

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(float maxRange, float damage) : maxRange(maxRange), damage(damage)
{
}

bool LightningBolt::use(Actor* owner, Actor* wearer)
{
	// find closest enemy (inside a maximum range)
	Actor* closestMonster = engine.get_closest_monster(wearer->posX, wearer->posY, maxRange);

	if (!closestMonster)
	{
		engine.gui->log_message(HPBARMISSING_PAIR, "No enemy is close enough to strike.");

		return false;
	}

	// hit closest monster for <damage> hit points
	engine.gui->log_message(HPBARFULL_PAIR, "A lighting bolt strikes the %s with a loud thunder!\n"
		"The damage is %g hit points.", closestMonster->name, damage);
	closestMonster->destructible->take_damage(closestMonster, damage);

	return Pickable::use(owner, wearer);
}

void LightningBolt::load(TCODZip& zip) 
{
	maxRange = zip.getFloat();
	damage = zip.getFloat();
}

void LightningBolt::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::LIGHTNING_BOLT));
	zip.putFloat(maxRange);
	zip.putFloat(damage);
}

//==Fireball==
Fireball::Fireball(float range, float damage) : LightningBolt(range, damage) {}

bool Fireball::use(Actor* owner, Actor* wearer)
{
	engine.gui->log_message(WHITE_PAIR, "Left-click a target tile for the fireball,\nor right-click to cancel.");

	int x, y;

	if (!engine.pick_tile(&x, &y)) // <-- runs a while loop here
	{
		return false;
	}

	// burn everything in <range> (including player)
	engine.gui->log_message(WHITE_PAIR, "The fireball explodes, burning everything within %g tiles!", Fireball::maxRange);

	// make impact explosion using a circle algorithm and curses library 
	// (this is a bit of a hack, but it works)
	
	// get the center of the explosion
	int centerX = x;
	int centerY = y;

	// get the radius of the explosion
	int radius = static_cast<int>(Fireball::maxRange);

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
	for (int numLoops = 0;numLoops < 1000; numLoops++ ) 
	{
		// draw the explosion
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				mvwaddch(explosionWindow, j, i, 'x' | COLOR_PAIR(rand() % 8));
			}
			int randColor = rand() % 7 + 1;
			wattron(explosionWindow, COLOR_PAIR(randColor));
			mvwaddch(explosionWindow, 0, 0 + i , 'x');
			wattroff(explosionWindow, COLOR_PAIR(randColor));
			wrefresh(explosionWindow);
		}
	}
	nodelay(explosionWindow, false);
	// draw the explosion
	wrefresh(explosionWindow);
	
	// wait for a bit using curses
	napms(1000);

	for (Actor* actor : engine.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->is_dead()
			&&
			actor->get_distance(x, y) <= Fireball::maxRange
			)
		{
			engine.gui->log_message(WHITE_PAIR, "The %s gets burned for %g hit points.", actor->name, damage);
			animation(actor->posX, actor->posY, maxRange);
		}
	}

	for (Actor* actor : engine.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->is_dead()
			&&
			actor->get_distance(x, y) <= Fireball::maxRange
			)
		{
			actor->destructible->take_damage(actor, damage);
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
		engine.render();

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
	maxRange = zip.getFloat();
	damage = zip.getFloat();
}

void Fireball::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::FIREBALL));
	zip.putFloat(maxRange);
	zip.putFloat(damage);
}

//==Confuser==
Confuser::Confuser(int nbTurns, float range) : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor* owner, Actor* wearer)
{
	engine.gui->log_message(WHITE_PAIR, "Left-click an enemy to confuse it,\nor right-click to cancel.");

	int x, y;

	if (!engine.pick_tile(&x, &y, range))
	{
		return false;
	}

	Actor* actor = engine.get_actor(x, y);

	if (!actor)
	{
		return false;
	}

	// replace the monster's AI with a confused one; 
	// after <nbTurns> turns the old AI will be restored
	ConfusedMonsterAi* confusedAi = new ConfusedMonsterAi(nbTurns, actor->ai);
	actor->ai = confusedAi;
	engine.gui->log_message(WHITE_PAIR, "The eyes of the %s look vacant,\nas he starts to stumble around!", actor->name);
	

	return Pickable::use(owner, wearer);
}

void Confuser::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	range = zip.getFloat();
}

void Confuser::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::CONFUSER));
	zip.putInt(nbTurns);
	zip.putFloat(range);
}

