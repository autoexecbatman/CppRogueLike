#include "TargetingSystem.h"

#include <curses.h>

#include "Game.h"

Vector2D TargetingSystem::select_target(Vector2D startPos, int maxRange)
{
	// Check if player is trying to attack but doesn't have a ranged weapon
	if (!game.player->has_state(ActorState::IS_RANGED))
	{
		game.message(WHITE_PAIR, "You can look around, but need a ranged weapon to attack at a distance!", true);
		// We still continue to the targeting mode, but will restrict actual attacks
	}

	Vector2D targetCursor = game.player->position;
	const Vector2D lastPosition = targetCursor;
	bool run = true;
	while (run)
	{
		clear();

		// Make the line follow the mouse position
		if (game.mouse_moved())
		{
			targetCursor = game.get_mouse_position();
		}
		game.render();

		// Display the FOV in white
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

		// First color the player position if the cursor has moved from the player position
		if (targetCursor != game.player->position)
		{
			mvchgat(lastPosition.y, lastPosition.x, 1, A_NORMAL, WHITE_PAIR, NULL);
		}

		//draw_range_indicator(startPos, maxRange);
		draw_los(targetCursor);

		// Check if the target is valid and update appearance accordingly
		bool validTarget = is_valid_target(startPos, targetCursor, maxRange);
		bool canAttack = validTarget && (game.player->has_state(ActorState::IS_RANGED));

		// Draw the target cursor - red if valid target and can attack, yellow if valid but cannot attack, blue if invalid
		int cursorColor = canAttack ? HPBARMISSING_PAIR : (validTarget ? GOLD_PAIR : WATER_PAIR);
		attron(COLOR_PAIR(cursorColor));
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(cursorColor));

		// If the cursor is on a monster then display the monster's name and stats
		const int distance = game.player->get_tile_distance(targetCursor);
		if (game.map->is_in_fov(targetCursor))
		{
			const auto& actor = game.map->get_actor(targetCursor);
			if (actor != nullptr)
			{
				// Display target information
				attron(COLOR_PAIR(actor->actorData.color));
				mvprintw(0, 0, "%s", actor->actorData.name.c_str());
				attroff(COLOR_PAIR(actor->actorData.color));

				// Print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->armorClass);
				mvprintw(3, 0, "Roll: %s", actor->attacker->roll.data());
				mvprintw(4, 0, "Str: %d", actor->strength);
				mvprintw(5, 0, "Dex: %d", actor->dexterity);
				mvprintw(6, 0, "Con: %d", actor->constitution);
				mvprintw(7, 0, "Int: %d", actor->intelligence);
				mvprintw(8, 0, "Wis: %d", actor->wisdom);
				mvprintw(9, 0, "Cha: %d", actor->charisma);

				// Print the distance from the player to the target cursor
				mvprintw(0, 50, "Distance: %d", distance);

				// Display dexterity missile attack adjustment if applicable
				if (game.player->has_state(ActorState::IS_RANGED) && game.player->dexterity > 0 &&
					game.player->dexterity <= game.dexterityAttributes.size()) {
					int missileAdj = game.dexterityAttributes[game.player->dexterity - 1].MissileAttackAdj;
					if (missileAdj != 0) {
						attron(COLOR_PAIR(LIGHTNING_PAIR));
						mvprintw(4, 50, "Ranged Attack Bonus: %+d", missileAdj);
						attroff(COLOR_PAIR(LIGHTNING_PAIR));
					}
				}

				// Add note about ranged attacks if examining a valid target
				if (validTarget && !game.player->has_state(ActorState::IS_RANGED))
				{
					attron(COLOR_PAIR(GOLD_PAIR));
					mvprintw(10, 0, "Need a ranged weapon to attack this target");
					attroff(COLOR_PAIR(GOLD_PAIR));
				}
			}
		}

		// Display legend at bottom
		mvprintw(MAP_HEIGHT - 2, 0, "Arrow keys: Move cursor | Enter: Select/Attack | Esc: Cancel");
		mvprintw(MAP_HEIGHT - 1, 0, "Targeting mode: %s",
			game.player->has_state(ActorState::IS_RANGED) ? "Attack" : "Examine");

		refresh();

		// Get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
			targetCursor.y--;
			break;
		case KEY_DOWN:
			targetCursor.y++;
			break;
		case KEY_LEFT:
			targetCursor.x--;
			break;
		case KEY_RIGHT:
			targetCursor.x++;
			break;
		case 10: // Enter key
			// If the key enter is pressed then select the target
			// Only allow attacking if we have a ranged weapon or aren't requiring it
			if (validTarget)
			{
				if (game.player->has_state(ActorState::IS_RANGED))
				{
					if (game.map->is_in_fov(targetCursor))
					{
						const auto& actor = game.map->get_actor(targetCursor);
						if (actor)
						{
							game.player->attacker->attack(*game.player, *actor);
							run = false;
							game.gameStatus = Game::GameStatus::NEW_TURN;
							return targetCursor;
						}
					}
				}
				else
				{
					// Player tried to attack but doesn't have a ranged weapon
					game.message(WHITE_PAIR, "You need a ranged weapon to attack at a distance!", true);
				}
			}
			break;
		case 'r':
		case 27: // Escape key
			// If the key escape is pressed then cancel the target selection
			run = false;
			break;
		}
	}
	clear();
	return Vector2D{ -1, -1 };
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