#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Creature.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Systems/MessageSystem.h"
#include "../Utils/Vector2D.h"
#include "AiMonster.h"
#include "AiSpider.h"

// Spider AI constants
constexpr int AMBUSH_DURATION = 5; // How long spiders stay in ambush mode
constexpr int AMBUSH_CHANCE = 30; // % chance to enter ambush mode when not seen
constexpr int POISON_COOLDOWN = 6; // Turns between poison attacks

//=============================================================================
// AiSpider Implementation
//=============================================================================

AiSpider::AiSpider(int poisonChance)
	: poisonChance(poisonChance)
{
}

void AiSpider::update(Creature& owner, GameContext& ctx)
{
	// Skip if spider is dead
	if (owner.is_dead())
	{
		return;
	}

	// Always ensure spiders have strength
	if (owner.get_strength() <= 0)
	{
		owner.set_strength(3); // Ensure minimum strength
	}

	// Reduce poison cooldown if active
	if (poisonCooldown > 0)
	{
		poisonCooldown--;
	}

	// Handle ambush behavior - this was missing in the previous fix
	if (isAmbushing)
	{
		ambushCounter--;

		// If player spots us while ambushing, or ambush time is up, stop ambushing
		if (ctx.map->is_in_fov(owner.position) || ambushCounter <= 0)
		{
			isAmbushing = false;

			// If player is close when we're discovered, get a surprise attack
			int distanceToPlayer = owner.get_tile_distance(ctx.player->position);
			if (ctx.map->is_in_fov(owner.position) && distanceToPlayer <= 3)
			{
				// Message about being ambushed
				ctx.messageSystem->message(owner.actorData.color, owner.actorData.name);
				ctx.messageSystem->message(WHITE_BLACK_PAIR, " ambushes you from hiding!", true);

				// If right next to player, get an immediate attack
				if (distanceToPlayer <= 1)
				{
					// Surprise attack gets a damage bonus - use proper damage system
					int normalDamage = owner.attacker->roll_damage(ctx.dice);
					int bonusDamage = ctx.dice->roll(1, 2); // Ambush damage bonus
					int totalDamage = normalDamage + bonusDamage;

					ctx.messageSystem->message(owner.actorData.color, owner.actorData.name);
					ctx.messageSystem->message(WHITE_BLACK_PAIR, " strikes with the element of surprise for ");
					ctx.messageSystem->message(WHITE_RED_PAIR, std::to_string(totalDamage));
					ctx.messageSystem->message(WHITE_BLACK_PAIR, " damage!", true);

					// Apply damage directly
					ctx.player->take_damage_and_check_death(totalDamage, ctx);

					// Also try for poison
					if (can_poison_attack(owner, ctx))
					{
						poison_attack(owner, *ctx.player, ctx);
					}
				}
			}
		}
		else
		{
			// Stay still while ambushing - don't reveal position
			return;
		}
	}
	else if (!ctx.map->is_in_fov(owner.position))
	{
		// Not in player's FOV, consider setting an ambush
		// Higher chance when player is near but doesn't see the spider
		int playerDistance = owner.get_tile_distance(ctx.player->position);
		int ambushChance = AMBUSH_CHANCE;

		// Increase chance when player is nearby but doesn't see us
		if (playerDistance <= 10)
		{
			ambushChance += 20; // Higher ambush chance when player is close
		}

		if (ctx.dice->d100() <= ambushChance)
		{
			// Find a good ambush position
			Vector2D ambushPos = find_ambush_position(owner, ctx.player->position, ctx);

			if (ambushPos.x != -1 && !ctx.map->get_actor(ambushPos, ctx)) // Valid position found and not occupied
			{
				// Move to ambush position
				owner.position = ambushPos;
				isAmbushing = true;
				ambushCounter = AMBUSH_DURATION;

				// Debug log
				ctx.messageSystem->log("Spider setting ambush at " + std::to_string(ambushPos.x) + "," + std::to_string(ambushPos.y));

				return;
			}
		}
	}

	if (blocked_by_sanctuary(ctx))
	{
		return;
	}

	// Special check for being adjacent to player - DIRECT ATTACK CODE
	int distanceToPlayer = owner.get_tile_distance(ctx.player->position);
	if (distanceToPlayer <= 1 && ctx.map->is_in_fov(owner.position))
	{
		// Directly trigger attack
		ctx.messageSystem->log("Spider attempting attack with poison");

		// First do the regular attack
		owner.attacker->attack(*ctx.player, ctx);

		// Then try poison - now independent of the regular attack
		if (can_poison_attack(owner, ctx))
		{
			poison_attack(owner, *ctx.player, ctx);
		}

		return;
	}

	// Handle movement and other behaviors normally
	if (ctx.map->is_in_fov(owner.position))
	{
		// Player can see spider - set maximum tracking
		moveCount = TRACKING_TURNS;
	}
	else if (moveCount > 0)
	{
		// Player can't see spider but we're still tracking
		moveCount--;
	}

	// Movement logic
	if (moveCount > 0)
	{
		// Move toward player
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

void AiSpider::move_toward_player(Creature& owner, GameContext& ctx)
{
	// Get direction to player
	Vector2D dirToPlayer = ctx.player->position - owner.position;
	int dx = (dirToPlayer.x != 0) ? (dirToPlayer.x > 0 ? 1 : -1) : 0;
	int dy = (dirToPlayer.y != 0) ? (dirToPlayer.y > 0 ? 1 : -1) : 0;

	// Try to move in that direction
	Vector2D newPos = owner.position + Vector2D{ dx, dy };

	// Check if the move is valid
	if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
	{
		owner.position = newPos;
	}
	else
	{
		// Try horizontal move
		newPos = owner.position + Vector2D{ dx, 0 };
		if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
		{
			owner.position = newPos;
		}
		else
		{
			// Try vertical move
			newPos = owner.position + Vector2D{ 0, dy };
			if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
			{
				owner.position = newPos;
			}
		}
	}
}

void AiSpider::random_move(Creature& owner, GameContext& ctx)
{
	int dx = ctx.dice->roll(-1, 1);
	int dy = ctx.dice->roll(-1, 1);

	if (dx != 0 || dy != 0)
	{
		Vector2D newPos = owner.position + Vector2D{ dx, dy };
		if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
		{
			owner.position = newPos;
		}
	}
}

void AiSpider::move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	// Get distance to target
	int distanceToTarget = owner.get_tile_distance(targetPosition);

	// If adjacent to target, attack
	if (distanceToTarget <= 1)
	{
		Creature* target = ctx.map->get_actor(targetPosition, ctx);
		if (target)
		{
			// Normal attack
			owner.attacker->attack(*target, ctx);

			// Try poison attack
			if (can_poison_attack(owner, ctx))
			{
				poison_attack(owner, *target, ctx);
			}
		}
		return;
	}

	// Spider prefers to move along walls if possible
	std::vector<Vector2D> possibleMoves;

	// Get possible moves (all adjacent tiles)
	for (int dy = -1; dy <= 1; dy++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if (dx == 0 && dy == 0)
			{
				continue; // Skip current position
			}

			Vector2D newPos = owner.position + Vector2D{ dx, dy };

			// Check if position is walkable and not occupied
			if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
			{
				// Check if this position is adjacent to a wall
				bool adjacentToWall = false;
				for (int wy = -1; wy <= 1; wy++)
				{
					for (int wx = -1; wx <= 1; wx++)
					{
						if (wx == 0 && wy == 0)
						{
							continue;
						}

						Vector2D wallCheck = newPos + Vector2D{ wx, wy };
						if (ctx.map->is_wall(wallCheck))
						{
							adjacentToWall = true;
							break;
						}
					}
					if (adjacentToWall)
					{
						break;
					}
				}

				// If adjacent to wall, add to list (higher priority)
				if (adjacentToWall)
				{
					possibleMoves.insert(possibleMoves.begin(), newPos);
				}
				else
				{
					possibleMoves.push_back(newPos);
				}
			}
		}
	}

	// If we have no moves, use default pathfinding
	if (possibleMoves.empty())
	{
		AiMonster::move_or_attack(owner, targetPosition, ctx);
		return;
	}

	// Find best move toward target
	Vector2D bestMove = owner.position;
	int bestDistance = std::numeric_limits<int>::max();

	for (const auto& move : possibleMoves)
	{
		int dist = std::abs(move.x - targetPosition.x) + std::abs(move.y - targetPosition.y);
		if (dist < bestDistance)
		{
			bestDistance = dist;
			bestMove = move;
		}
	}

	// Move to best position - final validation to prevent stacking
	if (bestMove != owner.position && !ctx.map->get_actor(bestMove, ctx))
	{
		owner.position = bestMove;
	}
}

bool AiSpider::can_poison_attack(Creature& owner, GameContext& ctx)
{
	// Check cooldown
	if (poisonCooldown > 0)
	{
		return false;
	}

	// Roll for poison chance (set at construction by spider type)
	return ctx.dice->d100() <= poisonChance;
}

void AiSpider::poison_attack(Creature& owner, Creature& target, GameContext& ctx)
{
	// Apply poison effect to target if it's the player
	if (&target == ctx.player)
	{
		// Calculate poison damage (1-3 points)
		int poisonDamage = ctx.dice->roll(1, 3);

		// Display poison message with damage amount
		ctx.messageSystem->message(RED_BLACK_PAIR, owner.actorData.name);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, " injects venom for ");
		ctx.messageSystem->message(WHITE_RED_PAIR, std::to_string(poisonDamage));
		ctx.messageSystem->message(WHITE_BLACK_PAIR, " extra poison damage!", true);

		// Deal the poison damage
		target.take_damage_and_check_death(poisonDamage, ctx);

		// Reset cooldown
		poisonCooldown = POISON_COOLDOWN;
	}
}

Vector2D AiSpider::find_ambush_position(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	// Look for positions near walls that are good for ambushing
	std::vector<Vector2D> candidates;

	// Search in a larger area around the current position (increased from 5x5 to 8x8)
	for (int y = -8; y <= 8; y++)
	{
		for (int x = -8; x <= 8; x++)
		{
			Vector2D pos = owner.position + Vector2D{ x, y };

			// Check boundaries
			if (pos.y < 0 ||
				pos.y >= ctx.map->get_height() ||
				pos.x < 0 ||
				pos.x >= ctx.map->get_width())
			{
				continue;
			}

			// Check if position is walkable, not occupied, and a good ambush spot
			if (ctx.map->can_walk(pos, ctx) &&
				!ctx.map->get_actor(pos, ctx) &&
				is_good_ambush_spot(pos, ctx))
			{
				// Evaluate position - closer to player's path is better for ambush
				int distToPlayer = std::abs(pos.x - targetPosition.x) + std::abs(pos.y - targetPosition.y);

				// Only consider positions that are not too far and not too close
				if (distToPlayer >= 3 && distToPlayer <= 12)
				{
					// Prioritize positions closer to doors, corners, or chokepoints
					candidates.push_back(pos);
				}
			}
		}
	}

	// Pick a random good position if available
	if (!candidates.empty())
	{
		int index = ctx.dice->roll(0, static_cast<int>(candidates.size()) - 1);
		return candidates.at(index);
	}
	else
	{
		// No good ambush position found
		return Vector2D{ -1, -1 };
	}
}

bool AiSpider::is_good_ambush_spot(Vector2D position, GameContext& ctx)
{
	// Good ambush spots are adjacent to walls (especially corners) and ideally in shadows
	int wallCount = 0;
	bool hasCorner = false;

	// Check the 8 surrounding tiles
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			if (x == 0 && y == 0)
			{
				continue; // Skip center
			}

			Vector2D adj = position + Vector2D{ x, y };

			// Check if this position is a wall
			if (ctx.map->is_wall(adj))
			{
				wallCount++;

				// Check if adjacent position forms a corner (has two walls next to it)
				int cornerWalls = 0;
				for (int cy = -1; cy <= 1; cy++)
				{
					for (int cx = -1; cx <= 1; cx++)
					{
						if (cx == 0 && cy == 0)
						{
							continue;
						}

						Vector2D cornerAdj = adj + Vector2D{ cx, cy };
						if (ctx.map->is_wall(cornerAdj))
						{
							cornerWalls++;
						}
					}
				}

				if (cornerWalls >= 3)
				{
					hasCorner = true;
				}
			}
		}
	}

	// Good ambush spots have at least 2 adjacent walls, better if it's a corner
	return wallCount >= 2 || hasCorner;
}

void AiSpider::load(const json& j)
{
	AiMonster::load(j);

	// Load AiSpider specific data
	if (j.contains("ambushCounter"))
	{
		ambushCounter = j.at("ambushCounter").get<int>();
	}

	if (j.contains("isAmbushing"))
	{
		isAmbushing = j.at("isAmbushing").get<bool>();
	}

	if (j.contains("poisonCooldown"))
	{
		poisonCooldown = j.at("poisonCooldown").get<int>();
	}

	if (j.contains("poisonChance"))
	{
		poisonChance = j.at("poisonChance").get<int>();
	}
}

void AiSpider::save(json& j)
{
	AiMonster::save(j);
	j["type"] = static_cast<int>(AiType::SPIDER);

	j["ambushCounter"] = ambushCounter;
	j["isAmbushing"] = isAmbushing;
	j["poisonCooldown"] = poisonCooldown;
	j["poisonChance"] = poisonChance;
}
