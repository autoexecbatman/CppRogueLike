#include "TargetingSystem.h"

#include <curses.h>

#include "Game.h"

Vector2D TargetingSystem::select_target(Vector2D startPos, int maxRange)
{
	//if (!game.player->has_state(ActorState::IS_RANGED))
	//{
	//	game.message(WHITE_PAIR, "You are not ranged!", true);
	//	return Vector2D{ -1,-1 };
	//}

	Vector2D targetCursor = game.player->position;
	const Vector2D lastPosition = targetCursor;
	bool run = true;
	while (run)
	{
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (game.mouse_moved())
		{
			targetCursor = game.get_mouse_position();
		}
		game.render();

		// display the FOV in white in row major order
		for (int y = 0; y < MAP_HEIGHT; y++)
		{
			for (int x = 0; x < MAP_WIDTH; x++)
			{
				if (game.map->is_in_fov(Vector2D{ y, x }))
				{
					mvchgat(y, x, 1, A_REVERSE, LIGHTNING_PAIR, NULL);
					refresh();
				}
			}
		}

		// first color the player position if the cursor has moved from the player position
		if (targetCursor != game.player->position)
		{
			mvchgat(lastPosition.y, lastPosition.x, 1, A_NORMAL, WHITE_PAIR, NULL);
		}

		draw_range_indicator(startPos, maxRange);

		draw_los(targetCursor);

		// Check if the target is valid and update appearance accordingly
		bool valid = is_valid_target(startPos, targetCursor, maxRange);

		// draw the target cursor

		attron(COLOR_PAIR(valid ? 6 : 4)); // Red if valid, blue if invalid
		/*attron(COLOR_PAIR(HPBARMISSING_PAIR));*/
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(valid ? 6 : 4));
		/*attroff(COLOR_PAIR(HPBARMISSING_PAIR));*/

		// if the cursor is on a monster then display the monster's name
		const int distance = game.player->get_tile_distance(targetCursor);
		if (game.map->is_in_fov(targetCursor))
		{
			const auto& actor = game.map->get_actor(targetCursor);
			// and actor is not an item
			if (actor != nullptr)
			{
				mvprintw(0, 0, actor->actorData.name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
				mvprintw(3, 0, "Roll: %s", actor->attacker->roll.data());
				// print the distance from the player to the target cursor
				mvprintw(0, 50, "Distance: %d", distance);
			}
		}

		refresh();

		// get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
			// move the selection cursor up
			targetCursor.y--;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			targetCursor.y++;
			break;

		case KEY_LEFT:
			// move the selection cursor left
			targetCursor.x--;
			break;

		case KEY_RIGHT:
			// move the selection cursor right
			targetCursor.x++;
			break;

		case 10:
			// if the key enter is pressed then select the target
			// and return the target position
			// if the target is a monster then attack it
		{
			if (is_valid_target(startPos, targetCursor, maxRange))
			{
				if (game.map->is_in_fov(targetCursor))
				{
					const auto& actor = game.map->get_actor(targetCursor);
					if (actor)
					{
						game.player->attacker->attack(*game.player, *actor);
						run = false;
						return targetCursor;
					}
				}
			}
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
		break;

		case 'r':
		case 27:
			// if the key escape is pressed then cancel the target selection
			run = false;
			break;

		default:
			break;
		} // end of switch (key)

	} // end of while (run)
	clear();
	return Vector2D{ -1,-1 };
}

void TargetingSystem::draw_los(Vector2D targetCursor)
{
	// draw a line using TCODLine class
	/*
	@CppEx
	// Going from point 5,8 to point 13,4
	int x = 5, y = 8;
	TCODLine::init(x, y, 13, 4);
	do {
		// update cell x,y
	} while (!TCODLine::step(&x, &y));
	*/
	Vector2D line{ 0,0 };
	TCODLine::init(game.player->position.x, game.player->position.y, targetCursor.x, targetCursor.y);
	while (!TCODLine::step(&line.x, &line.y))
	{
		mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_PAIR, NULL);
		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(line.y, line.x, '*');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));
	}

	//// draw the line using TCODPath
	//// first create a path from the player to the target cursor
	//if (targetCursor.y < MAP_HEIGHT && targetCursor.x < MAP_WIDTH)
	//{
	//	game.map->tcodPath->compute(game.player->position.x, game.player->position.y, targetCursor.x, targetCursor.y);
	//	// then iterate over the path
	//	int x, y;
	//	while (game.map->tcodPath->walk(&x, &y, true))
	//	{
	//		/*mvchgat(y, x, 1, A_REVERSE, WHITE_PAIR, NULL);*/
	//		attron(COLOR_PAIR(HPBARMISSING_PAIR));
	//		mvaddch(y, x, '*');
	//		attroff(COLOR_PAIR(HPBARMISSING_PAIR));
	//	}
	//}

	Vector2D from = game.player->position;
	Vector2D to = targetCursor;

	int x0 = from.x;
	int y0 = from.y;
	int x1 = to.x;
	int y1 = to.y;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	bool blocked = false;

	while (x0 != x1 || y0 != y1) {
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}

		// Skip the start and end points for visualization
		if ((x0 != from.x || y0 != from.y) && (x0 != to.x || y0 != to.y))
		{
			if (!game.map->can_walk(Vector2D{ y0, x0 })) 
			{
				blocked = true;
			}

			int colorPair = blocked ? 4 : 8; // Red if clear, blue if blocked
			attron(COLOR_PAIR(colorPair));
			mvaddch(y0, x0, '*');
			attroff(COLOR_PAIR(colorPair));
		}
	}
}

void TargetingSystem::draw_range_indicator(Vector2D center, int range)
{
	attron(COLOR_PAIR(5)); // Dim color for range indicator

	for (int y = center.y - range; y <= center.y + range; y++) {
		for (int x = center.x - range; x <= center.x + range; x++) {
			if (x >= 0 && x < game.map->get_width() && y >= 0 && y < game.map->get_height()) {
				// Check if this point is at the edge of the range
				float distance = std::sqrt(std::pow(x - center.x, 2) + std::pow(y - center.y, 2));
				if (std::abs(distance - range) < 0.5f) {
					mvaddch(y, x, 'X');
				}
			}
		}
	}

	attroff(COLOR_PAIR(5));
}

// Check if a target position is valid
bool TargetingSystem::is_valid_target(Vector2D from, Vector2D to, int maxRange)
{
	// Check if the target is within range
	if (from.distance_to(to) > maxRange)
	{
		return false;
	}

	// Check if there's a clear line of sight
	if (!game.map->has_los(from, to))
	{
		return false;
	}

	// Check if there's an enemy at the target
	auto entity = game.map->get_actor(to);
	if (!entity)
	{
		return false;
	}

	// Make sure it's not the player
	if (entity == game.player.get())
	{
		return false;
	}

	return true;
}

// Process a ranged attack
bool TargetingSystem::process_ranged_attack(Creature& attacker, Vector2D targetPos) {
	auto target = game.map->get_actor(targetPos);
	if (!target || target == &attacker)
	{
		return false;
	}

	// Check range and line of sight
	if (!is_valid_target(attacker.position, targetPos, 4))
	{
		return false;
	}

	// Get projectile info from the player
	char projectileSymbol = '*';
	int projectileColor = COLOR_PAIR(2);

	// Animate the projectile
	animate_projectile(attacker.position, targetPos, projectileSymbol, projectileColor);

	//// Perform the attack
	//target->takeDamage(attacker->getAttackDamage());

	return true;
}

// Animate a projectile flying from source to target
void TargetingSystem::animate_projectile(Vector2D from, Vector2D to, char projectileSymbol, int colorPair)
{
	// Calculate the path using Bresenham's algorithm
	int x0 = from.x;
	int y0 = from.y;
	int x1 = to.x;
	int y1 = to.y;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	std::vector<Vector2D> path;

	// Generate the path
	while (x0 != x1 || y0 != y1) {
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}

		// Skip the start point
		if (x0 != from.x || y0 != from.y) {
			path.push_back(Vector2D{ y0, x0 });
		}

		// Stop at walls
		if (!game.map->can_walk(Vector2D{ y0, x0 }) && (x0 != to.x || y0 != to.y)) {
			break;
		}
	}

	// Redraw the map before animation
	game.render();
	refresh();

	// Animate along the path
	for (const auto& pos : path) {
		// Skip the end point for rendering
		if (pos.x == to.x && pos.y == to.y) {
			continue;
		}

		// Determine projectile orientation based on direction
		char symbol = projectileSymbol;
		if (pos.x > from.x && std::abs(pos.x - from.x) > std::abs(pos.y - from.y)) {
			symbol = '>'; // Right
		}
		else if (pos.x < from.x && std::abs(pos.x - from.x) > std::abs(pos.y - from.y)) {
			symbol = '<'; // Left
		}
		else if (pos.y < from.y) {
			symbol = '^'; // Up
		}
		else if (pos.y > from.y) {
			symbol = 'v'; // Down
		}

		// Draw projectile
		attron(COLOR_PAIR(colorPair));
		mvaddch(pos.y, pos.x, symbol);
		attroff(COLOR_PAIR(colorPair));

		refresh();
		napms(50); // Control speed of projectile (lower = faster)

		// Erase the projectile (by redrawing the map tile)
		if (game.map->can_walk(pos)) {
			mvaddch(pos.y, pos.x, '.');
		}
		else 
		{
			mvaddch(pos.y, pos.x, '#');
		}
	}

	// Visual effect at impact point
	attron(COLOR_PAIR(2)); // Red for impact
	mvaddch(to.y, to.x, '*');
	attroff(COLOR_PAIR(2));
	refresh();
	napms(100); // Brief pause for impact

	// Redraw the target
	auto entity = game.map->get_actor(to);
	if (entity) {
		entity->render();
		refresh();
	}

};