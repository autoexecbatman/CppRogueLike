#include "Fireball.h"
#include "LightningBolt.h"
#include "Actor.h"
#include "Game.h"
#include "Colors.h"

//==Fireball==
Fireball::Fireball(int range, int damage) : LightningBolt(range, damage) {}

bool Fireball::use(Actor& owner, Actor& wearer)
{
	int x{ 0 };
	int y{ 0 };

	if (!game.pick_tile(&x, &y, Fireball::maxRange)) // <-- runs a while loop here
	{
		return false;
	}

	// burn everything in <range> (including player)
	game.appendMessagePart(WHITE_PAIR, std::format("The fireball explodes, burning everything within {} tiles!", Fireball::maxRange));
	game.finalizeMessage();

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

void Fireball::animation(int x, int y, int maxRange)
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