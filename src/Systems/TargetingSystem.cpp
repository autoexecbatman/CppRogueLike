#include "TargetingSystem.h"

#include <curses.h>

#include "../Game.h"
#include "../Core/GameContext.h"

const Vector2D TargetingSystem::select_target(GameContext& ctx, Vector2D startPos, int maxRange) const
{
	// Check if player is trying to attack but doesn't have a ranged weapon
	if (!ctx.player->has_state(ActorState::IS_RANGED))
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "You can look around, but need a ranged weapon to attack at a distance!", true);
		// We still continue to the targeting mode, but will restrict actual attacks
	}

	Vector2D targetCursor = ctx.player->position;
	const Vector2D lastPosition = targetCursor;
	bool run = true;
	while (run)
	{
		clear();

		// Make the line follow the mouse position
		if (ctx.input_handler->mouse_moved())
		{
			targetCursor = ctx.input_handler->get_mouse_position();
		}
		ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);

		// Display the FOV in white
		for (int y = 0; y < MAP_HEIGHT; y++)
		{
			for (int x = 0; x < MAP_WIDTH; x++)
			{
				if (ctx.map->is_in_fov(Vector2D{ y, x }))
				{
					mvchgat(y, x, 1, A_REVERSE, WHITE_BLUE_PAIR, NULL);
					/*refresh();*/
				}
			}
		}

		// First color the player position if the cursor has moved from the player position
		if (targetCursor != ctx.player->position)
		{
			mvchgat(lastPosition.y, lastPosition.x, 1, A_NORMAL, WHITE_BLACK_PAIR, NULL);
		}

		//draw_range_indicator(ctx, startPos, maxRange);
		draw_los(ctx, targetCursor);

		// Check if the target is valid and update appearance accordingly
		bool validTarget = is_valid_target(ctx, startPos, targetCursor, maxRange);
		bool canAttack = validTarget && (ctx.player->has_state(ActorState::IS_RANGED));

		// Draw the target cursor - red if valid target and can attack, yellow if valid but cannot attack, blue if invalid
		int cursorColor = canAttack ? WHITE_RED_PAIR : (validTarget ? YELLOW_BLACK_PAIR : BLUE_BLACK_PAIR);
		attron(COLOR_PAIR(cursorColor));
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(cursorColor));

		// If the cursor is on a monster then display the monster's name and stats
		const int distance = ctx.player->get_tile_distance(targetCursor);
		if (ctx.map->is_in_fov(targetCursor))
		{
			const auto& actor = ctx.map->get_actor(targetCursor);
			if (actor != nullptr)
			{
				// Display target information
				attron(COLOR_PAIR(actor->actorData.color));
				mvprintw(0, 0, "%s", actor->actorData.name.c_str());
				attroff(COLOR_PAIR(actor->actorData.color));

				// Print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->get_hp(), actor->destructible->get_max_hp());
				mvprintw(2, 0, "AC: %d", actor->destructible->get_armor_class());
				mvprintw(3, 0, "Roll: %s", actor->attacker->get_roll().c_str());
				mvprintw(4, 0, "Str: %d", actor->get_strength());
				mvprintw(5, 0, "Dex: %d", actor->get_dexterity());
				mvprintw(6, 0, "Con: %d", actor->get_constitution());
				mvprintw(7, 0, "Int: %d", actor->get_intelligence());
				mvprintw(8, 0, "Wis: %d", actor->get_wisdom());
				mvprintw(9, 0, "Cha: %d", actor->get_charisma());

				// Print the distance from the player to the target cursor
				mvprintw(0, 50, "Distance: %d", distance);

				// Display dexterity missile attack adjustment if applicable
				if (ctx.player->has_state(ActorState::IS_RANGED)
					&&
					ctx.player->get_dexterity() > 0
					&&
					ctx.player->get_dexterity() <= ctx.data_manager->get_dexterity_attributes().size())
				{
					int missileAdj = ctx.data_manager->get_dexterity_attributes()[ctx.player->get_dexterity() - 1].MissileAttackAdj;
					if (missileAdj != 0) {
						attron(COLOR_PAIR(WHITE_BLUE_PAIR));
						mvprintw(4, 50, "Ranged Attack Bonus: %+d", missileAdj);
						attroff(COLOR_PAIR(WHITE_BLUE_PAIR));
					}
				}

				// Add note about ranged attacks if examining a valid target
				if (validTarget && !ctx.player->has_state(ActorState::IS_RANGED))
				{
					attron(COLOR_PAIR(YELLOW_BLACK_PAIR));
					mvprintw(10, 0, "Need a ranged weapon to attack this target");
					attroff(COLOR_PAIR(YELLOW_BLACK_PAIR));
				}
			}
		}

		// Display legend at bottom
		mvprintw(MAP_HEIGHT - 2, 0, "Arrow keys: Move cursor | Enter: Select/Attack | Esc: Cancel");
		mvprintw(MAP_HEIGHT - 1, 0, "Targeting mode: %s",
			ctx.player->has_state(ActorState::IS_RANGED) ? "Attack" : "Examine");

		//refresh();

		// Get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
		case 'w':
		case 'W':
			targetCursor.y--;
			break;
		case KEY_DOWN:
		case 's':
		case 'S':
			targetCursor.y++;
			break;
		case KEY_LEFT:
		case 'a':
		case 'A':
			targetCursor.x--;
			break;
		case KEY_RIGHT:
		case 'd':
		case 'D':
			targetCursor.x++;
			break;
		case 10: // Enter key
			// If the key enter is pressed then select the target
			// Only allow attacking if we have a ranged weapon or aren't requiring it
			if (validTarget)
			{
				if (ctx.player->has_state(ActorState::IS_RANGED))
				{
					if (ctx.map->is_in_fov(targetCursor))
					{
						const auto& actor = ctx.map->get_actor(targetCursor);
						if (actor)
						{
							ctx.player->attacker->attack(*ctx.player, *actor, ctx);
							run = false;
							*ctx.game_status = static_cast<int>(1);  // Game::GameStatus::NEW_TURN
							return targetCursor;
						}
					}
				}
				else
				{
					// Player tried to attack but doesn't have a ranged weapon
					ctx.message_system->message(WHITE_BLACK_PAIR, "You need a ranged weapon to attack at a distance!", true);
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
		refresh();

		// Restore the game display after targeting
		ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
		return Vector2D{ -1, -1 };
}

void TargetingSystem::draw_los(const GameContext& ctx, Vector2D targetCursor) const
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
	TCODLine::init(ctx.player->position.x, ctx.player->position.y, targetCursor.x, targetCursor.y);
	while (!TCODLine::step(&line.x, &line.y))
	{
		mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_BLACK_PAIR, NULL);
		attron(COLOR_PAIR(WHITE_RED_PAIR));
		mvaddch(line.y, line.x, '*');
		attroff(COLOR_PAIR(WHITE_RED_PAIR));
	}

	//// draw the line using TCODPath
	//// first create a path from the player to the target cursor
	//if (targetCursor.y < MAP_HEIGHT && targetCursor.x < MAP_WIDTH)
	//{
	//	game.map.tcodPath->compute(game.player->position.x, game.player->position.y, targetCursor.x, targetCursor.y);
	//	// then iterate over the path
	//	int x, y;
	//	while (game.map.tcodPath->walk(&x, &y, true))
	//	{
	//		/*mvchgat(y, x, 1, A_REVERSE, WHITE_BLACK_PAIR, NULL);*/
	//		attron(COLOR_PAIR(WHITE_RED_PAIR));
	//		mvaddch(y, x, '*');
	//		attroff(COLOR_PAIR(WHITE_RED_PAIR));
	//	}
	//}

	Vector2D from = ctx.player->position;
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
			if (!ctx.map->can_walk(Vector2D{ y0, x0 }))
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

void TargetingSystem::draw_range_indicator(GameContext& ctx, Vector2D center, int range) const
{
	attron(COLOR_PAIR(5)); // Dim color for range indicator

	for (int y = center.y - range; y <= center.y + range; y++) {
		for (int x = center.x - range; x <= center.x + range; x++) {
			if (x >= 0 && x < ctx.map->get_width() && y >= 0 && y < ctx.map->get_height()) {
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
bool TargetingSystem::is_valid_target(const GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const
{
	// Check if the target is within range
	if (from.distance_to(to) > maxRange)
	{
		return false;
	}

	// Check if there's a clear line of sight
	if (!ctx.map->has_los(from, to))
	{
		return false;
	}

	// Check if there's an enemy at the target
	auto entity = ctx.map->get_actor(to);
	if (!entity)
	{
		return false;
	}

	// Make sure it's not the player
	if (entity == ctx.player)
	{
		return false;
	}

	return true;
}

// Process a ranged attack
bool TargetingSystem::process_ranged_attack(GameContext& ctx, Creature& attacker, Vector2D targetPos) const {
	auto target = ctx.map->get_actor(targetPos);
	if (!target || target == &attacker)
	{
		return false;
	}

	// Check range and line of sight
	if (!is_valid_target(ctx, attacker.position, targetPos, 4))
	{
		return false;
	}

	// Get projectile info from the player
	char projectileSymbol = '*';
	int projectileColor = COLOR_PAIR(2);

	// Animate the projectile
	animate_projectile(ctx, attacker.position, targetPos, projectileSymbol, projectileColor);

	//// Perform the attack
	//target->takeDamage(attacker->getAttackDamage());

	return true;
}

// Animate a projectile flying from source to target
void TargetingSystem::animate_projectile(GameContext& ctx, Vector2D from, Vector2D to, char projectileSymbol, int colorPair) const
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
		if (!ctx.map->can_walk(Vector2D{ y0, x0 }) && (x0 != to.x || y0 != to.y)) {
			break;
		}
	}

	// Redraw the map before animation
	ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);
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
		if (ctx.map->can_walk(pos)) {
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
	auto entity = ctx.map->get_actor(to);
	if (entity) {
		entity->render();
		refresh();
	}

}

// AOE preview from pick_tile method
void TargetingSystem::draw_aoe_preview(Vector2D center, int radius) const
{
	const int sideLength = radius * 2 + 1;
	const int chebyshevD = std::max(abs(center.x - (center.x - radius)), abs(center.y - (center.y - radius)));
	const int height = sideLength;
	const int width = sideLength;

	// Calculate the position of the aoe window
	Vector2D centerOfExplosion = center - Vector2D{ chebyshevD, chebyshevD };

	// draw the AOE in white
	for (int tilePosX = center.x - chebyshevD; tilePosX < centerOfExplosion.x + width; tilePosX++)
	{
		for (int tilePosY = center.y - chebyshevD; tilePosY < centerOfExplosion.y + height; tilePosY++)
		{
			mvchgat(tilePosY, tilePosX, 1, A_REVERSE, WHITE_BLUE_PAIR, nullptr);
		}
	}
}

// Game.cpp compatibility method - preserves exact behavior
bool TargetingSystem::pick_tile(GameContext& ctx, Vector2D* position, int maxRange) const
{
	// the target cursor is initialized at the player's position
	Vector2D targetCursor = ctx.player->position;
	bool run = true;
	while (run)
	{
		// CRITICAL FIX: Clear screen to remove previous targeting overlays
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (ctx.input_handler->mouse_moved())
		{
			targetCursor = ctx.input_handler->get_mouse_position();
		}
		ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);

		// first color the player position if the cursor has moved from the player position
		if (targetCursor != ctx.player->position)
		{
			mvchgat(ctx.player->position.y, ctx.player->position.x, 1, A_NORMAL, WHITE_BLACK_PAIR, nullptr);
		}

		// draw a line using TCODLine class
		Vector2D line{ 0,0 };
		TCODLine::init(ctx.player->position.x, ctx.player->position.y, targetCursor.x, targetCursor.y);
		while (!TCODLine::step(&line.x, &line.y))
		{
			mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_BLACK_PAIR, nullptr);
		}

		attron(COLOR_PAIR(WHITE_RED_PAIR));
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(WHITE_RED_PAIR));

		// if the cursor is on a monster then display the monster's name
		if (ctx.map->is_in_fov(targetCursor))
		{
			const auto& actor = ctx.map->get_actor(targetCursor);
			// CRITICAL FIX: Don't write directly to main screen - this causes bleeding
			// Store info for later display or use a separate window
			if (actor != nullptr)
			{
				// These direct writes to (0,0) cause screen bleeding - commenting out for now
				// mvprintw(0, 0, actor->actorData.name.c_str());
				// mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				// mvprintw(2, 0, "AC: %d", actor->destructible->dr);
			}
		}
		
		// highlight the possible range of the explosion make it follow the cursor
		draw_aoe_preview(targetCursor, maxRange);
		refresh();

		// get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
		case 'w':
		case 'W':
			// move the selection cursor up
			targetCursor.y--;
			break;

		case KEY_DOWN:
		case 's':
		case 'S':
			// move the selection cursor down
			targetCursor.y++;
			break;

		case KEY_LEFT:
		case 'a':
		case 'A':
			// move the selection cursor left
			targetCursor.x--;
			break;

		case KEY_RIGHT:
		case 'd':
		case 'D':
			// move the selection cursor right
			targetCursor.x++;
			break;

		case 'f':
			// if the player presses the 'f' key
			// then the target selection is confirmed
			// and the target coordinates are returned

			// first display a message
			ctx.message_system->message(WHITE_BLACK_PAIR, "Target confirmed", true);
			// then return the coordinates
			*position = targetCursor;

			// Restore game display before returning
			ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
			return true;
			break;

		case 10:
			// if the key enter is pressed then select the target
			// and return the target position
			ctx.message_system->message(WHITE_BLACK_PAIR, "Attack confirmed", true);
			// if the target is a monster then attack it
		{
			if (ctx.map->is_in_fov(targetCursor))
			{
				const auto& actor = ctx.map->get_actor(targetCursor);
				// and actor is not an item
				if (actor != nullptr)
				{
					ctx.player->attacker->attack(*ctx.player, *actor, ctx);
					// Restore game display after attack
					ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
					run = false;
				}
			}
		}
		break;
		case 'r':
		case 27:
			ctx.message_system->message(WHITE_BLACK_PAIR, "Target selection canceled", true);
			// Restore game display before exit
			ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
			run = false;
			break;

		default:break;
		}

	}

	return false;
}

// Handle ranged attack coordination
void TargetingSystem::handle_ranged_attack(GameContext& ctx) const
{
	// When attackMode is true, we require a ranged weapon for attacks
	// When attackMode is false, we're just examining and don't require a ranged weapon

	// Enter targeting mode with appropriate requirements
	Vector2D targetPos = select_target(ctx, ctx.player->position, 4);

	// If a valid target was selected and we're in attack mode
	if (targetPos.x != -1 && targetPos.y != -1) {
		// Process the ranged attack (including projectile animation)
		process_ranged_attack(ctx, *ctx.player, targetPos);
	}
}