#include <cmath>
#include <memory>
#include <vector>

#include "../Actor/Creature.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Objects/Web.h"
#include "../Systems/MessageSystem.h"
#include "../Utils/Vector2D.h"
#include "AiWebSpinner.h"

constexpr int WEB_COOLDOWN = 8;
constexpr int WEB_MIN_SIZE = 3;
constexpr int MAX_WEBS = 5; // Maximum number of webs that can exist at once per spider
constexpr int WEB_MAX_SIZE = 5;
constexpr int WEB_STRENGTH = 3;

AiWebSpinner::AiWebSpinner(int poisonChance)
	: AiSpider(poisonChance)
{
}

void AiWebSpinner::update(Creature& owner, GameContext& ctx)
{
	// Always ensure spiders have strength
	if (owner.get_strength() <= 0)
	{
		owner.set_strength(4); // Ensure minimum strength
	}

	// Update cooldowns
	if (webCooldown > 0)
	{
		webCooldown--;
	}
	if (poisonCooldown > 0)
	{
		poisonCooldown--;
	}

	// DIRECT ATTACK CODE - Check if player is adjacent
	int distanceToPlayer = owner.get_tile_distance(ctx.player->position);
	if (distanceToPlayer <= 1 && ctx.map->is_in_fov(owner.position))
	{
		// Directly trigger attack - avoid any inheritance issues
		ctx.messageSystem->log("Web spinner attempting attack with poison");

		// First do the regular attack
		owner.attacker->attack(*ctx.player, ctx);

		// Then check for poison - independent of the regular attack success
		if (can_poison_attack(owner, ctx))
		{
			poison_attack(owner, *ctx.player, ctx);
		}

		// Skip web spinning and other behaviors if we're attacking
		return;
	}

	// Web spinning logic - only if not attacking
	if (webCooldown == 0 && should_create_web(owner, ctx))
	{
		if (try_create_web(owner, ctx))
		{
			webCooldown = WEB_COOLDOWN;

			// Show message about web spinning
			ctx.messageSystem->message(owner.actorData.color, owner.actorData.name);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, " spins a sticky web!", true);

			return;
		}
	}

	// Fall back to standard movement behavior
	if (ctx.map->is_in_fov(owner.position))
	{
		// If player can see us, move toward player
		moveCount = TRACKING_TURNS;
		move_toward_player(owner, ctx);
	}
	else if (moveCount > 0)
	{
		// Still tracking player
		moveCount--;
		move_toward_player(owner, ctx);
	}
	else
	{
		// Occasional random movement
		if (ctx.dice->d20() == 1)
		{
			random_move(owner, ctx);
		}
	}
}

bool AiWebSpinner::should_create_web(Creature& owner, GameContext& ctx)
{
	// If player is directly adjacent, don't create web (attack instead)
	int distToPlayer = owner.get_tile_distance(ctx.player->position);
	if (distToPlayer <= 1)
	{
		return false;
	}

	// If spider has already laid a web recently, reduce chance of creating another
	if (has_laid_web())
	{
		// Reduced chance if already laid a web (20% instead of 40%)
		if (distToPlayer <= 10 && ctx.dice->d100() < 20)
		{
			return true;
		}
	}
	else
	{
		// Higher chance if hasn't laid a web yet
		// If player is nearby, moderate chance to create defensive web
		if (distToPlayer <= 10 && ctx.dice->d100() < 40)
		{
			return true;
		}
	}

	// Even if player is far, occasional web creation for traps (10% chance)
	if (ctx.dice->d100() < 10)
	{
		return true;
	}

	// Count actual webs in the game objects
	int webCount = 0;
	for (const auto& obj : *ctx.objects)
	{
		if (obj && obj->actorData.name == "spider web")
		{
			webCount++;
		}
	}

	// Allow up to MAX_WEBS per spider (default is 5)
	return webCount < MAX_WEBS;
}

bool AiWebSpinner::try_create_web(Creature& owner, GameContext& ctx)
{
	// Determine the web size - bigger webs when player is closer
	int distToPlayer = owner.get_tile_distance(ctx.player->position);
	int webSize = WEB_MAX_SIZE;

	if (distToPlayer < 5)
	{
		// Maximum size defensive web
		webSize = WEB_MAX_SIZE;
	}
	else if (distToPlayer < 15)
	{
		// Medium size web
		webSize = WEB_MIN_SIZE + 1;
	}
	else
	{
		// Minimum size trap web
		webSize = WEB_MIN_SIZE;
	}

	// Center the web at the spider's position or at a strategic location
	Vector2D webCenter;

	if (ctx.map->is_in_fov(owner.position) && distToPlayer < 10)
	{
		// If player can see spider, create web between spider and player
		int dx = ctx.player->position.x - owner.position.x;
		int dy = ctx.player->position.y - owner.position.y;

		if (dx != 0)
		{
			dx = dx / std::abs(dx);
		}
		if (dy != 0)
		{
			dy = dy / std::abs(dy);
		}

		// Position web to block player's path
		webCenter = owner.position + Vector2D{ dx * 2, dy * 2 };

		// Make sure center is in bounds
		webCenter.x = std::max(0, std::min(webCenter.x, ctx.map->get_width() - 1));
		webCenter.y = std::max(0, std::min(webCenter.y, ctx.map->get_height() - 1));
	}
	else
	{
		// If player can't see spider, create web at a nearby location
		webCenter = owner.position;
	}

	// Generate the web pattern - now creating actual Web entities
	generate_web_entities(webCenter, webSize, ctx);

	// Dramatic message about web creation
	ctx.messageSystem->message(RED_YELLOW_PAIR, owner.actorData.name);
	if (webSize >= WEB_MAX_SIZE - 1)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, " creates a massive web network!", true);
	}
	else
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, " spins a complex web structure!", true);
	}

	// Mark this spider as having laid a web - now using our own method
	set_web_laid(true);

	return true;
}

// Helper method to check if a position is valid for placing a web
bool AiWebSpinner::is_valid_web_position(Vector2D pos, GameContext& ctx)
{
	// Check bounds
	if (pos.y < 0 ||
		pos.y >= ctx.map->get_height() ||
		pos.x < 0 ||
		pos.x >= ctx.map->get_width())
	{
		return false;
	}

	// Check if the position is walkable
	if (!ctx.map->can_walk(pos, ctx))
	{
		return false;
	}

	// Don't place webs on occupied tiles
	if (ctx.map->get_actor(pos, ctx) != nullptr)
	{
		return false;
	}

	// Check if there's already a web at this position
	for (const auto& obj : *ctx.objects)
	{
		if (obj && obj->position == pos && obj->actorData.name == "spider web")
		{
			return false;
		}
	}

	return true;
}

void AiWebSpinner::generate_web_entities(Vector2D center, int size, GameContext& ctx)
{
	// Create a complex web pattern centered at the given position
	// with size determining the radius/complexity

	// Different web patterns
	enum class WebPattern
	{
		CIRCULAR,
		SPIRAL,
		RADIAL,
		CHAOTIC
	};

	// Choose a random pattern
	WebPattern pattern = static_cast<WebPattern>(ctx.dice->roll(0, 3));

	// Track positions where we want to create webs
	std::vector<Vector2D> webPositions;

	switch (pattern)
	{
	case WebPattern::CIRCULAR:
		// Create a circular/oval web
		for (int y = -size; y <= size; y++)
		{
			for (int x = -size; x <= size; x++)
			{

				float normalizedDist = ((float)(x * x) / (size * size)) + ((float)(y * y) / (size * size));

				// Create web with higher density near the edges
				if (normalizedDist <= 1.0f &&
					(normalizedDist >= 0.7f || ctx.dice->d100() < 40))
				{

					Vector2D pos = center + Vector2D{ x, y };
					if (is_valid_web_position(pos, ctx))
					{
						webPositions.push_back(pos);
					}
				}
			}
		}
		break;

	case WebPattern::SPIRAL:
		// Create a spiral pattern
		{
			float angle = 0.0f;
			float radiusStep = (float)size / 20.0f;

			for (int i = 0; i < 20 * size; i++)
			{
				float radius = radiusStep * i;
				if (radius > size)
				{
					break;
				}

				int x = center.x + (int)(radius * cos(angle));
				int y = center.y + (int)(radius * sin(angle));

				Vector2D pos{ x, y };
				if (is_valid_web_position(pos, ctx))
				{
					webPositions.push_back(pos);
				}

				angle += 0.5f;

				// Add some random offshoots from the spiral
				if (ctx.dice->d100() < 30)
				{
					for (int j = 1; j <= 3; j++)
					{
						int offX = x + ctx.dice->roll(-1, 1);
						int offY = y + ctx.dice->roll(-1, 1);

						Vector2D offPos{ offX, offY };
						if (is_valid_web_position(offPos, ctx))
						{
							webPositions.push_back(offPos);
						}
					}
				}
			}
		}
		break;

	case WebPattern::RADIAL:
		// Create a radial web with spokes and connecting threads
		{
			// First create the spokes
			int numSpokes = 6 + ctx.dice->roll(0, 4); // 6-10 spokes

			for (int i = 0; i < numSpokes; i++)
			{
				float angle = static_cast<float>(i) * (2.0f * 3.14159f / numSpokes);

				for (int dist = 1; dist <= size; dist++)
				{
					int x = center.x + (int)(dist * cos(angle));
					int y = center.y + (int)(dist * sin(angle));

					Vector2D pos{ x, y };
					if (is_valid_web_position(pos, ctx))
					{
						webPositions.push_back(pos);
					}
				}
			}

			// Now create concentric circles connecting the spokes
			for (int radius = 1; radius <= size; radius += 1 + (size / 5))
			{
				for (float angle = 0; angle < 2.0f * 3.14159f; angle += 0.2f)
				{
					int x = center.x + (int)(radius * cos(angle));
					int y = center.y + (int)(radius * sin(angle));

					Vector2D pos{ x, y };
					if (is_valid_web_position(pos, ctx))
					{
						webPositions.push_back(pos);
					}
				}
			}
		}
		break;

	case WebPattern::CHAOTIC:
		// Create a chaotic, asymmetric web
		{
			// Start with a dense center
			for (int y = -2; y <= 2; y++)
			{
				for (int x = -2; x <= 2; x++)
				{
					if (abs(x) + abs(y) <= 3)
					{
						Vector2D pos = center + Vector2D{ x, y };
						if (is_valid_web_position(pos, ctx))
						{
							webPositions.push_back(pos);
						}
					}
				}
			}

			// Then create random strands extending outward
			for (int strand = 0; strand < 8 + ctx.dice->roll(0, 7); strand++)
			{
				Vector2D strandPos = center;
				int strandLength = ctx.dice->roll(3, size);

				for (int step = 0; step < strandLength; step++)
				{
					// Random direction but with bias toward continuing current direction
					int dx = ctx.dice->roll(-1, 1);
					int dy = ctx.dice->roll(-1, 1);

					strandPos.x += dx;
					strandPos.y += dy;

					if (is_valid_web_position(strandPos, ctx))
					{
						webPositions.push_back(strandPos);
					}
					else
					{
						break; // Hit a wall or invalid position
					}

					// Occasionally branch the strand
					if (ctx.dice->d100() < 30)
					{
						Vector2D branchPos = strandPos;
						int branchLength = ctx.dice->roll(2, 4);

						for (int bStep = 0; bStep < branchLength; bStep++)
						{
							branchPos.x += ctx.dice->roll(-1, 1);
							branchPos.y += ctx.dice->roll(-1, 1);

							if (is_valid_web_position(branchPos, ctx))
							{
								webPositions.push_back(branchPos);
							}
							else
							{
								break;
							}
						}
					}
				}
			}
		}
		break;
	}

	// Create Web entities at these positions
	for (const auto& pos : webPositions)
	{
		// Set variable web strength
		int webStrength = WEB_STRENGTH;
		if (ctx.dice->d100() < 25)
		{
			// Some webs are stronger or weaker
			webStrength += ctx.dice->roll(-1, 2);
		}

		// Create a new Web entity
		auto web = std::make_unique<Web>(pos, webStrength, *ctx.tileConfig);
		ctx.objects->emplace_back(std::move(web));
	}
}

void AiWebSpinner::load(const json& j)
{
	AiSpider::load(j);

	// Load AiWebSpinner specific data
	if (j.contains("webCooldown"))
	{
		webCooldown = j.at("webCooldown").get<int>();
	}
}

void AiWebSpinner::save(json& j)
{
	AiSpider::save(j);
	j["type"] = static_cast<int>(AiType::WEB_SPINNER); // overwrite SPIDER written by AiSpider::save

	j["webCooldown"] = webCooldown;
}