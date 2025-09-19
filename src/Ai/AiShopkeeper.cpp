// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"
#include "../Items/Items.h"
#include "../Menu/MenuTrade.h"
#include "../ActorTypes/Player.h"

constexpr auto TRACKING_TURNS = 3; // Used in AiShopkeeper::update()
constexpr auto MAX_TRADE_DISTANCE = 1;  // Maximum distance to initiate a trade (adjacent only - no diagonals)

AiShopkeeper::AiShopkeeper() : moveCount(0), tradeMenuOpen(false), hasApproachedPlayer(false) {}

// If positionDifference > 0, return 1; otherwise, return -1
int AiShopkeeper::calculate_step(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}

void AiShopkeeper::moveToTarget(Actor& owner, Vector2D target)
{
    // Calculate direction vector using Vector2D operations
    Vector2D direction = target - owner.position;  // Assumes Vector2D has subtraction operator
    
    // Calculate step directions for each axis
    Vector2D step{
        direction.y == 0 ? 0 : (direction.y > 0 ? 1 : -1),  // Y step direction
        direction.x == 0 ? 0 : (direction.x > 0 ? 1 : -1)   // X step direction
    };
    
    // Define movement priorities using Vector2D directly
    std::vector<Vector2D> moves{
        step,                    // First priority: diagonal movement (both Y and X)
        Vector2D{0, step.x},            // Second priority: horizontal only (X movement)
        Vector2D{step.y, 0}             // Third priority: vertical only (Y movement)
    };
    
    // Try each move in priority order
    for (const auto& move : moves)
    {
        Vector2D nextPos = owner.position + move;  // Vector2D addition
        
        // ULTIMATE FIX: Multiple collision checks to prevent any overlap
        if (game.map.can_walk(nextPos))
        {
            // EXTRA SAFETY: Explicit player collision check
            if (game.player && game.player->position == nextPos)
            {
                continue; // Skip this move - player is there
            }
            
            // EXTRA SAFETY: Double-check no actor at position
            if (game.map.get_actor(nextPos) != nullptr)
            {
                continue; // Skip this move - position occupied
            }
            
            // SAFE TO MOVE - Update position using Vector2D
            owner.position = nextPos;
            break; // Exit after successful move
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
	game.menu_manager.set_should_take_input(false);
}

void AiShopkeeper::update(Creature& owner)
{
	// CRITICAL FIX: Detect and repair broken shopkeepers from old system
	if (!owner.shop)
	{
		game.log("Detected broken shopkeeper - repairing with shop component");
		// Create shop component for broken shopkeeper
		owner.shop = std::make_unique<ShopKeeper>(ShopType::GENERAL_STORE, ShopQuality::AVERAGE);
	}
	
	// Skip if dead
	if (owner.ai == nullptr || owner.destructible->is_dead())
	{
		return;
	}

	// Reset trade menu flag when player moves away
	int distance = owner.get_tile_distance(game.player->position);
	if (distance > MAX_TRADE_DISTANCE)
	{
		tradeMenuOpen = false;
	}

	// If shopkeeper has already approached player once, don't approach again
	if (hasApproachedPlayer)
	{
		return; // Stay passive after first approach
	}

	// Check if player is visible and not already tracking
	if (game.map.is_in_fov(owner.position))
	{
		// Only start tracking if we're not already tracking (prevents constant following)
		if (moveCount == 0)
		{
			moveCount = TRACKING_TURNS;
			hasApproachedPlayer = true; // Mark that shopkeeper has now approached
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
		// Only move, don't trade during approach (trade check handled above)
		moveToTarget(owner, game.player->position);
	}
}

void AiShopkeeper::load(const json& j)
{
	moveCount = j.at("moveCount").get<int>();
	if (j.contains("tradeMenuOpen"))
	{
		tradeMenuOpen = j.at("tradeMenuOpen").get<bool>();
	}
	if (j.contains("hasApproachedPlayer"))
	{
		hasApproachedPlayer = j.at("hasApproachedPlayer").get<bool>();
	}
}

void AiShopkeeper::save(json& j)
{
	j["type"] = static_cast<int>(AiType::SHOPKEEPER);
	j["moveCount"] = moveCount;
	j["tradeMenuOpen"] = tradeMenuOpen;
	j["hasApproachedPlayer"] = hasApproachedPlayer;
}
// file: AiShopkeeper.cpp
