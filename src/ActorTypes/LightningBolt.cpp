#include <memory>
#include <curses.h>
#include <libtcod.h>
#include <random>
#include <cmath>

#include "LightningBolt.h"
#include "../Game.h"
#include "../Colors/Colors.h"

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(int maxRange, int damage) noexcept : maxRange(maxRange), damage(damage) {}

bool LightningBolt::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// find closest enemy (inside a maximum range)
	const auto& closestMonster = ctx.creature_manager->get_closest_monster(*ctx.creatures, wearer.position, maxRange);

	if (!closestMonster)
	{
		ctx.message_system->message(WHITE_RED_PAIR, "No enemy is close enough to strike.", true);
		return false;
	}
	else
	{
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "A lightning bolt strikes the ");
		ctx.message_system->append_message_part(WHITE_BLUE_PAIR, closestMonster->actorData.name);
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " with a loud thunder!");
		ctx.message_system->finalize_message();

		// Animate the lightning bolt effect
		animate_lightning(wearer.position, closestMonster->position, ctx);

		ctx.message_system->message(WHITE_RED_PAIR, std::format("The damage is {} hit points.", damage), true);
		closestMonster->destructible->take_damage(*closestMonster, damage, ctx);

		// Clean up dead creatures after damage
		ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);

		return Pickable::use(owner, wearer, ctx);
	}
}

void LightningBolt::animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
{
	// Random generator for lightning jaggedness
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> jitterDist(-1, 1);

	// Calculate direct path
	std::vector<Vector2D> mainPath;

	// Use Bresenham's algorithm to get direct path
	int x0 = from.x;
	int y0 = from.y;
	int x1 = to.x;
	int y1 = to.y;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	// Generate the main path
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

		if (x0 != from.x || y0 != from.y) {
			mainPath.push_back(Vector2D{ y0, x0 });
		}
	}

	// Lightning bolt characters
	const char LIGHTNING_CHARS[] = { '/', '\\', '|', '-', '*', '+' };
	const int CHAR_COUNT = 6;
	std::uniform_int_distribution<> charDist(0, CHAR_COUNT - 1);

	// Create lightning flash
	clear();
	ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
	ctx.gui->gui_render(ctx);
	
	// Lightning phases
	const int FLASH_COUNT = 3;
	
	for (int flash = 0; flash < FLASH_COUNT; flash++) {
		// Generate jagged lightning path with branches
		std::vector<Vector2D> lightningPath = mainPath;
		
		// Add jitter to make the bolt jagged
		for (size_t i = 1; i < lightningPath.size() - 1; i++) {
			// Every few segments, add jitter
			if (i % 2 == 0) {
				lightningPath[i].x += jitterDist(gen);
				lightningPath[i].y += jitterDist(gen);
			}
		}
		
		// Draw lightning path
		attron(COLOR_PAIR(WHITE_BLUE_PAIR));
		for (const auto& pos : lightningPath) {
			// Choose random lightning character for jagged effect
			char symbol = LIGHTNING_CHARS[charDist(gen)];
			mvaddch(pos.y, pos.x, symbol);
		}
		
		// Add branch lightning (smaller offshoots)
		int branchCount = 2 + flash;  // More branches in later flashes
		std::uniform_int_distribution<> branchPosDist(0, static_cast<int>(lightningPath.size()) - 1);
		std::uniform_int_distribution<> branchLengthDist(2, 5);
		
		for (int b = 0; b < branchCount; b++) {
			// Choose a random point along the main path to branch from
			if (lightningPath.size() > 3) {
				int branchPos = branchPosDist(gen);
				Vector2D branchStart = lightningPath[branchPos];
				
				// Generate branch in a random direction
				int branchLength = branchLengthDist(gen);
				int dirX = jitterDist(gen);
				int dirY = jitterDist(gen);
				
				// Ensure we have a direction (not zero)
				if (dirX == 0 && dirY == 0) dirX = 1;
				
				Vector2D current = branchStart;
				for (int i = 0; i < branchLength; i++) {
					current.x += dirX;
					current.y += dirY;
					
					// Add some randomness to branch path
					if (i > 0 && i % 2 == 0) {
						current.x += jitterDist(gen);
						current.y += jitterDist(gen);
					}
					
					// Draw branch segment if it's within bounds
					if (current.x >= 0 && current.y >= 0 &&
						current.x < ctx.map->get_width() && current.y < ctx.map->get_height()) {
						char symbol = LIGHTNING_CHARS[charDist(gen)];
						mvaddch(current.y, current.x, symbol);
					}
				}
			}
		}
		
		// Create a bright flash at target position
		mvaddch(to.y, to.x, '@');
		
		attroff(COLOR_PAIR(WHITE_BLUE_PAIR));
		refresh();
		
		// Flash timing - quick for lightning
		napms(70);
		
		// Clear for next flash
		if (flash < FLASH_COUNT - 1) {
			clear();
			ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
			ctx.gui->gui_render(ctx);
			napms(50); // Brief darkness between flashes
		}
	}
	
	// End with a final impact flash
	attron(COLOR_PAIR(WHITE_BLUE_PAIR));
	// Draw impact markers around target
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			if (dx == 0 && dy == 0) {
				mvaddch(to.y, to.x, '*');
			} else {
				int x = to.x + dx;
				int y = to.y + dy;
				if (x >= 0 && y >= 0 && x < ctx.map->get_width() && y < ctx.map->get_height()) {
					mvaddch(y, x, '.');
				}
			}
		}
	}
	attroff(COLOR_PAIR(WHITE_BLUE_PAIR));
	refresh();
	napms(150);

	// Redraw game
	clear();
	ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
	ctx.gui->gui_render(ctx);
	refresh();
}

void LightningBolt::load(const json& j)
{
	maxRange = j["maxRange"];
	damage = j["damage"];
}

void LightningBolt::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::LIGHTNING_BOLT);
	j["maxRange"] = maxRange;
	j["damage"] = damage;
}