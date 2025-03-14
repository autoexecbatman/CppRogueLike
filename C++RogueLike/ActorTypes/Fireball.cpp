// File: Fireball.cpp
#include <gsl/util>
#include "Fireball.h"
#include "LightningBolt.h"
#include "../Actor/Actor.h"
#include "../Game.h"
#include "../Colors/Colors.h"

//==Fireball==
Fireball::Fireball(int range, int damage) : LightningBolt(range, damage) {}

bool Fireball::use(Item& owner, Creature& wearer)
{
	Vector2D tilePicked{ 0, 0 };

	if (!game.pick_tile(&tilePicked, Fireball::maxRange)) // <-- runs a while loop here
	{
		return false;
	}

	// burn everything in <range> (including player)
	game.appendMessagePart(WHITE_PAIR, std::format("The fireball explodes, burning everything within {} tiles!", Fireball::maxRange));
	game.finalizeMessage();

	// make impact explosion using a circle algorithm and curses library 
	// (this is a bit of a hack, but it works)

	// get the center of the explosion
	Vector2D center = tilePicked;

	// get the radius of the explosion
	int radius = Fireball::maxRange;

	int sideLength = radius * 2 + 1;

	// calculate the chebyshev distance from the player to maxRange
	int chebyshev = std::max(abs(center.x - (center.x - radius)), abs(center.y - (center.y - radius)));

	int height = sideLength;
	int width = sideLength;

	Vector2D centerOfExplosion{center.y - chebyshev, center.x - chebyshev};

	WINDOW* explosionWindow = newwin(
		height, // number of rows
		width, // number of columns
		centerOfExplosion.y, // y position
		centerOfExplosion.x // x position
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

	// if the player is in range of the fireball animate the player
	if (game.player->get_tile_distance(tilePicked) <= Fireball::maxRange)
	{
		animation(game.player->position, maxRange);
		// damage the player
		game.player->destructible->take_damage(*game.player, damage);
	}

	for (const auto& c : game.creatures)
	{
		if (c)
		{
			if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
			{
				/*game.gui->log_message(WHITE_PAIR, "The %s gets burned!\nfor %d hp.", c->actorData.name.c_str(), damage);*/
				game.appendMessagePart(WHITE_PAIR, std::format("The {} gets burned!", c->actorData.name));
				game.appendMessagePart(WHITE_PAIR, std::format("for {} hp.", damage));
				game.finalizeMessage();
				animation(c->position, maxRange);
			}
		}
	}

	for (const auto& c : game.creatures)
	{
		if (c)
		{
			if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
			{
				c->destructible->take_damage(*c, damage);
			}
		}
	}

	return Pickable::use(owner, wearer);
}

void Fireball::animation(Vector2D position, int maxRange)
{
	bool run = true;
	while (run == true)
	{
		clear();
		game.render();

		attron(COLOR_PAIR(FIREBALL_PAIR));
		mvprintw(position.y, position.x, "~");
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

void Fireball::load(const json& j)
{
	maxRange = j["maxRange"].get<int>();
	damage = j["damage"].get<int>();
}

void Fireball::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::FIREBALL);
	j["maxRange"] = maxRange;
	j["damage"] = damage;
}