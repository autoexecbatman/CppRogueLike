#include <memory>
#include <curses.h>
#include <libtcod.h>

#include "Game.h"
#include "Colors.h"
#include "LightningBolt.h"

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(int maxRange, int damage) : maxRange(maxRange), damage(damage) {}

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
	// print to stdscr PDC
	clear();
	mvprintw(0, 0, "A lighting bolt strikes the %s with a loud thunder!\n", closestMonster->name.c_str());
	refresh();
	getch();
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