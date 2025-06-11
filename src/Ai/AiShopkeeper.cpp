// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"
#include "../Items.h"
#include "../Menu/MenuTrade.h"
#include "../ActorTypes/Player.h"

constexpr auto TRACKING_TURNS = 3; // Used in AiShopkeeper::update()
constexpr auto MAX_TRADE_DISTANCE = 1;  // Maximum distance to initiate a trade (adjacent only - no diagonals)
constexpr auto COOLDOWN_TURNS = 10; // Turns to wait after interaction before approaching again

AiShopkeeper::AiShopkeeper() : moveCount(0), cooldownCount(0), tradeMenuOpen(false) {}

// If positionDifference > 0, return 1; otherwise, return -1
int AiShopkeeper::calculate_step(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}

void AiShopkeeper::moveToTarget(Actor& owner, Vector2D target)
{
	// Calculate how many squares away the target is horizontally and vertically
	int moveX = target.x - owner.position.x; // Number of squares to move horizontally
	int moveY = target.y - owner.position.y; // Number of squares to move vertically

	// Decide in which direction to take one step horizontally and vertically
	int stepX = calculate_step(moveX); // Direction to move horizontally (left or right)
	int stepY = calculate_step(moveY); // Direction to move vertically (up or down)

	// List potential moves in order of priority
	std::vector<Vector2D> moves
	{
		{stepY,stepX},  // First priority: move diagonally
		{0, stepX},     // Second priority: move horizontally only 
		{stepY, 0}      // Third priority: move vertically only
	};

	// Try each move in order of priority until one is successful
	for (const auto& move : moves)
	{
		int nextX = owner.position.x + move.x;
		int nextY = owner.position.y + move.y;
		Vector2D nextPos{ nextY, nextX };
		
		// ULTIMATE FIX: Multiple collision checks to prevent any overlap
		if (game.map->can_walk(nextPos))
		{
			// EXTRA SAFETY: Explicit player collision check
			if (game.player && game.player->position == nextPos)
			{
				continue; // Skip this move - player is there
			}
			
			// EXTRA SAFETY: Double-check no actor at position
			if (game.map->get_actor(nextPos) != nullptr)
			{
				continue; // Skip this move - position occupied
			}
			
			// SAFE TO MOVE
			owner.position.x = nextX;
			owner.position.y = nextY;
			break; // Exit the loop after a successful move
		}
	}
	// If no valid moves available, shopkeeper stays in place
}

void AiShopkeeper::moveOrTrade(Creature& shopkeeper, Vector2D target)
{
	// Use unified distance function - no coordinate confusion!
	const int distance = shopkeeper.get_tile_distance(target);
	
	// DEBUG: Log distance calculation
	game.log("Shopkeeper distance to player: " + std::to_string(distance) + 
		" (Shopkeeper: " + std::to_string(shopkeeper.position.x) + "," + std::to_string(shopkeeper.position.y) + 
		" Player: " + std::to_string(target.x) + "," + std::to_string(target.y) + ")");
	
	// Only trade if adjacent (distance exactly 1.0) and menu not already open
	if (distance <= MAX_TRADE_DISTANCE && !shopkeeper.destructible->is_dead() && !tradeMenuOpen)
	{
		game.log("TRADE CONDITION MET: Opening trade menu!");
		trade(shopkeeper, *game.player);
		tradeMenuOpen = true; // Mark that trade menu is open
		// Start cooldown after trading
		cooldownCount = COOLDOWN_TURNS;
		moveCount = 0; // Stop tracking immediately
	}
	else if (distance > MAX_TRADE_DISTANCE)
	{
		game.log("Distance > MAX_TRADE_DISTANCE, moving toward player");
		// Only move if not in cooldown and distance is greater than trade distance
		moveToTarget(shopkeeper, target);
	}
	else
	{
		game.log("Not trading: distance=" + std::to_string(distance) + 
			" dead=" + std::to_string(shopkeeper.destructible->is_dead()) + 
			" menuOpen=" + std::to_string(tradeMenuOpen));
	}
}

void AiShopkeeper::trade(Creature& shopkeeper, Creature& player)
{
	game.menus.push_back(std::make_unique<MenuTrade>(shopkeeper, player));
	game.shouldInput = false;
}

void AiShopkeeper::update(Creature& owner)
{
	// Skip if dead
	if (owner.ai == nullptr || owner.destructible->is_dead())
	{
		return;
	}

	// Handle cooldown - don't approach player during cooldown
	if (cooldownCount > 0)
	{
		cooldownCount--;
		// Reset trade menu flag when cooldown ends
		if (cooldownCount == 0)
		{
			tradeMenuOpen = false;
		}
		return; // Do nothing during cooldown
	}

	// Check if player is visible and not already tracking
	if (game.map->is_in_fov(owner.position))
	{
		// Only start tracking if we're not already tracking (prevents constant following)
		if (moveCount == 0)
		{
			moveCount = TRACKING_TURNS;
		}
	}
	else if (moveCount > 0)
	{
		// Player not visible but still tracking - countdown
		moveCount--;
	}

	// Move towards player only if actively tracking
	if (moveCount > 0)
	{
		// Clean Vector2D usage - no coordinate confusion!
		moveOrTrade(owner, game.player->position);
	}
}

void AiShopkeeper::load(const json& j)
{
	moveCount = j.at("moveCount").get<int>();
	if (j.contains("cooldownCount"))
	{
		cooldownCount = j.at("cooldownCount").get<int>();
	}
	if (j.contains("tradeMenuOpen"))
	{
		tradeMenuOpen = j.at("tradeMenuOpen").get<bool>();
	}
}

void AiShopkeeper::save(json& j)
{
	j["type"] = static_cast<int>(AiType::SHOPKEEPER);
	j["moveCount"] = moveCount;
	j["cooldownCount"] = cooldownCount;
	j["tradeMenuOpen"] = tradeMenuOpen;
}
// file: AiShopkeeper.cpp
