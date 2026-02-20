#include <cmath>
#include <limits>

#include "AiSpider.h"
#include "../ActorTypes/Monsters/Spider.h"
#include "../Map/Map.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../ActorTypes/Player.h"
#include "../Systems/MessageSystem.h"

// Spider AI constants
constexpr int AMBUSH_DURATION = 5;      // How long spiders stay in ambush mode
constexpr int AMBUSH_CHANCE = 30;       // % chance to enter ambush mode when not seen
constexpr int POISON_DURATION = 3;      // Duration of poison effect in turns
constexpr int POISON_COOLDOWN = 6;      // Turns between poison attacks
constexpr int WEB_COOLDOWN = 8;         // Turns between web creation

// Web structure constants
constexpr int WEB_MIN_SIZE = 3;     // Minimum web size (diameter)
constexpr int WEB_MAX_SIZE = 5;     // Maximum web size (diameter)
constexpr int WEB_STRENGTH = 3;     // Web strength (turns to escape)
constexpr int WEB_TRAP_CHANCE = 40; // Chance (%) for instant immobilization

// Web tile definition - you would need to add this to your TileType enum in Map.h
// enum class TileType { ..., WEB, ... };

//=============================================================================
// AiSpider Implementation
//=============================================================================

void AiSpider::update(Creature& owner, GameContext& ctx)
{
    // Skip if spider is dead
    if (owner.destructible->is_dead())
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
                ctx.message_system->message(owner.actorData.color, owner.actorData.name);
                ctx.message_system->message(WHITE_BLACK_PAIR, " ambushes you from hiding!", true);

                // If right next to player, get an immediate attack
                if (distanceToPlayer <= 1)
                {
                    // Surprise attack gets a damage bonus - use proper damage system
                    int normalDamage = owner.attacker->roll_damage(ctx.dice);
                    int bonusDamage = ctx.dice->roll(1, 2); // Ambush damage bonus
                    int totalDamage = normalDamage + bonusDamage;

                    ctx.message_system->message(owner.actorData.color, owner.actorData.name);
                    ctx.message_system->message(WHITE_BLACK_PAIR, " strikes with the element of surprise for ");
                    ctx.message_system->message(WHITE_RED_PAIR, std::to_string(totalDamage));
                    ctx.message_system->message(WHITE_BLACK_PAIR, " damage!", true);

                    // Apply damage directly
                    ctx.player->destructible->take_damage(*ctx.player, totalDamage, ctx);

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
                ctx.message_system->log("Spider setting ambush at " + std::to_string(ambushPos.x) + "," + std::to_string(ambushPos.y));

                return;
            }
        }
    }

    // Special check for being adjacent to player - DIRECT ATTACK CODE
    int distanceToPlayer = owner.get_tile_distance(ctx.player->position);
    if (distanceToPlayer <= 1 && ctx.map->is_in_fov(owner.position))
    {
        // Directly trigger attack
        ctx.message_system->log("Spider attempting attack with poison");

        // First do the regular attack
        owner.attacker->attack(owner, *ctx.player, ctx);

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
            owner.attacker->attack(owner, *target, ctx);

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
            if (dx == 0 && dy == 0) continue; // Skip current position

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
                        if (wx == 0 && wy == 0) continue;

                        Vector2D wallCheck = newPos + Vector2D{ wx, wy };
                        if (ctx.map->is_wall(wallCheck))
                        {
                            adjacentToWall = true;
                            break;
                        }
                    }
                    if (adjacentToWall) break;
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

    // Get spider-specific poison chance
    Spider* spider = dynamic_cast<Spider*>(&owner);
    if (!spider)
    {
        return false;
    }

    // Roll for poison chance
    int poisonChance = spider->get_poison_chance();
    return (ctx.dice->d100() <= poisonChance);
}

void AiSpider::poison_attack(Creature& owner, Creature& target, GameContext& ctx)
{
    // Apply poison effect to target if it's the player
    if (&target == ctx.player)
    {
        // Calculate poison damage (1-3 points)
        int poisonDamage = ctx.dice->roll(1, 3);

        // Display poison message with damage amount
        ctx.message_system->message(RED_BLACK_PAIR, owner.actorData.name);
        ctx.message_system->message(WHITE_BLACK_PAIR, " injects venom for ");
        ctx.message_system->message(WHITE_RED_PAIR, std::to_string(poisonDamage));
        ctx.message_system->message(WHITE_BLACK_PAIR, " extra poison damage!", true);

        // Deal the poison damage
        target.destructible->take_damage(target, poisonDamage, ctx);

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
            if (pos.y < 0
                ||
                pos.y >= ctx.map->get_height()
                ||
                pos.x < 0
                ||
                pos.x >= ctx.map->get_width())
            {
                continue;
            }

            // Check if position is walkable, not occupied, and a good ambush spot
            if (ctx.map->can_walk(pos, ctx)
                &&
                !ctx.map->get_actor(pos, ctx)
                &&
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
            if (x == 0 && y == 0) continue; // Skip center

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
                        if (cx == 0 && cy == 0) continue;

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
}

void AiSpider::save(json& j)
{
    AiMonster::save(j);

    // Save AiSpider specific data
    j["ambushCounter"] = ambushCounter;
    j["isAmbushing"] = isAmbushing;
    j["poisonCooldown"] = poisonCooldown;
}

//=============================================================================
// AiWebSpinner Implementation
//=============================================================================

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
        ctx.message_system->log("Web spinner attempting attack with poison");

        // First do the regular attack
        owner.attacker->attack(owner, *ctx.player, ctx);

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
            ctx.message_system->message(owner.actorData.color, owner.actorData.name);
            ctx.message_system->message(WHITE_BLACK_PAIR, " spins a sticky web!", true);

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
    // Cast owner to Spider to access spider-specific methods
    Spider* spider = dynamic_cast<Spider*>(&owner);
    if (!spider)
    {
        ctx.message_system->log("Error: tryCreateWeb called on non-Spider creature");
        return false;
    }

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

        if (dx != 0) dx = dx / std::abs(dx);
        if (dy != 0) dy = dy / std::abs(dy);

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
    ctx.message_system->message(RED_YELLOW_PAIR, owner.actorData.name);
    if (webSize >= WEB_MAX_SIZE - 1)
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, " creates a massive web network!", true);
    }
    else
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, " spins a complex web structure!", true);
    }

    // Mark this spider as having laid a web - now using our own method
    set_web_laid(true);

    return true;
}

// Helper method to check if a position is valid for placing a web
bool AiWebSpinner::is_valid_web_position(Vector2D pos, GameContext& ctx)
{
    // Check bounds
    if (pos.y < 0
        ||
        pos.y >= ctx.map->get_height()
        ||
        pos.x < 0
        ||
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
            if (radius > size) break;

            int x = center.x + (int)(radius * cos(angle));
            int y = center.y + (int)(radius * sin(angle));

            Vector2D pos{ y, x };
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

                    Vector2D offPos{ offY, offX };
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
            float angle = (float)i * (2.0f * 3.14159f / numSpokes);

            for (int dist = 1; dist <= size; dist++)
            {
                int x = center.x + (int)(dist * cos(angle));
                int y = center.y + (int)(dist * sin(angle));

                Vector2D pos{ y, x };
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

                Vector2D pos{ y, x };
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
        auto web = std::make_unique<Web>(pos, webStrength);
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

    // Save AiWebSpinner specific data
    j["webCooldown"] = webCooldown;
}