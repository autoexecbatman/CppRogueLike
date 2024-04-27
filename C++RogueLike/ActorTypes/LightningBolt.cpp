#include <memory>
#include <curses.h>
#include <libtcod.h>

#include "Game.h"
#include "Colors.h"
#include "LightningBolt.h"

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(int maxRange, int damage) noexcept : maxRange(maxRange), damage(damage) {}

bool LightningBolt::use(Actor& owner, Actor& wearer)
{
	// find closest enemy (inside a maximum range)
	Actor* closestMonster = game.get_closest_monster(wearer.posX, wearer.posY, maxRange);

	if (!closestMonster)
	{
		game.message(HPBARMISSING_PAIR, "No enemy is close enough to strike.", true);

		return false;
	}
	else
	{
		closestMonster->destructible->take_damage(*closestMonster, damage);

		clear();
		mvprintw(0, 0, "A lighting bolt strikes the %s with a loud thunder!\n", closestMonster->name.c_str());
		refresh();
		getch();
		game.message(HPBARMISSING_PAIR, std::format("The damage is {} hit points.", damage), true);

		return Pickable::use(owner, wearer);
	}

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