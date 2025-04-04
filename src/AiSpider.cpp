#include <cmath>
#include "AiSpider.h"
#include "Game.h"
#include "Spider.h"
#include "Map/Map.h"
#include "Colors/Colors.h"

// Spider AI constants
const int AMBUSH_DURATION = 5;      // How long spiders stay in ambush mode
const int AMBUSH_CHANCE = 30;       // % chance to enter ambush mode when not seen
const int POISON_DURATION = 3;      // Duration of poison effect in turns
const int POISON_COOLDOWN = 6;      // Turns between poison attacks
const int WEB_COOLDOWN = 8;         // Turns between web creation

// Web structure constants
const int WEB_MIN_SIZE = 3;     // Minimum web size (diameter)
const int WEB_MAX_SIZE = 5;     // Maximum web size (diameter)
const int WEB_STRENGTH = 3;     // Web strength (turns to escape)
const int WEB_TRAP_CHANCE = 40; // Chance (%) for instant immobilization

// Web tile definition - you would need to add this to your TileType enum in Map.h
// enum class TileType { ..., WEB, ... };

//=============================================================================
// AiSpider Implementation
//=============================================================================

AiSpider::AiSpider() : AiMonster(), ambushCounter(0), isAmbushing(false), poisonCooldown(0)
{
}

void AiSpider::update(Creature& owner)
{
    // Cast the owner to a Spider if possible
    Spider* spider = dynamic_cast<Spider*>(&owner);

    if (!spider || owner.destructible->is_dead())
    {
        return;
    }

    // Decrease cooldown counters
    if (poisonCooldown > 0)
    {
        poisonCooldown--;
    }

    // Check if spider is in player's FOV
    bool inPlayerFOV = game.map->is_in_fov(owner.position);

    // Handle ambush behavior
    if (isAmbushing)
    {
        ambushCounter--;

        // If the player spots us while ambushing, or ambush time is up, stop ambushing
        if (inPlayerFOV || ambushCounter <= 0)
        {
            isAmbushing = false;
            // If player spots us, get a bonus attack
            if (inPlayerFOV && game.player->get_tile_distance(owner.position) <= 1)
            {
                // Surprise attack on player
                owner.attacker->attack(owner, *game.player);

                // Try poison attack
                if (canPoisonAttack(owner))
                {
                    poisonAttack(owner, *game.player);
                }
            }
        }
        else
        {
            // Stay still while ambushing
            return;
        }
    }
    else if (!inPlayerFOV && game.d.d100() <= AMBUSH_CHANCE)
    {
        // Not seen by player, chance to enter ambush mode
        Vector2D ambushPos = findAmbushPosition(owner, game.player->position);

        if (ambushPos.x != -1)
        {
            // Move to ambush position
            owner.position = ambushPos;
            isAmbushing = true;
            ambushCounter = AMBUSH_DURATION;
            return;
        }
    }

    // Regular monster behavior if not ambushing
    if (inPlayerFOV)
    {
        // Player can see spider - set maximum tracking
        moveCount = TRACKING_TURNS;
    }
    else
    {
        // Player can't see spider
        if (moveCount > 0)
        {
            // If we were previously tracking the player, decrement counter
            moveCount--;
        }
    }

    // Behavior based on distance and visibility
    if (moveCount > 0)
    {
        // Recently seen player - actively pursue
        moveOrAttack(owner, game.player->position);
    }
    else if (game.d.d20() == 1)  // Occasional random movement for distant spiders
    {
        // Random movement in one of eight directions
        int dx = game.d.roll(-1, 1);
        int dy = game.d.roll(-1, 1);

        if (dx != 0 || dy != 0)  // Ensure we're not standing still
        {
            Vector2D newPos = owner.position + Vector2D{ dy, dx };
            if (game.map->can_walk(newPos) && !game.map->get_actor(newPos))
            {
                owner.position = newPos;
            }
        }
    }
}

void AiSpider::moveOrAttack(Creature& owner, Vector2D targetPosition)
{
    // Get distance to target
    int distanceToTarget = owner.get_tile_distance(targetPosition);

    // If adjacent to target, attack
    if (distanceToTarget <= 1)
    {
        Creature* target = game.map->get_actor(targetPosition);
        if (target)
        {
            // Normal attack
            owner.attacker->attack(owner, *target);

            // Try poison attack
            if (canPoisonAttack(owner))
            {
                poisonAttack(owner, *target);
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

            Vector2D newPos = owner.position + Vector2D{ dy, dx };

            // Check if position is walkable and not occupied
            if (game.map->can_walk(newPos) && !game.map->get_actor(newPos))
            {
                // Check if this position is adjacent to a wall
                bool adjacentToWall = false;
                for (int wy = -1; wy <= 1; wy++)
                {
                    for (int wx = -1; wx <= 1; wx++)
                    {
                        if (wx == 0 && wy == 0) continue;

                        Vector2D wallCheck = newPos + Vector2D{ wy, wx };
                        if (game.map->is_wall(wallCheck))
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
        AiMonster::moveOrAttack(owner, targetPosition);
        return;
    }

    // Find best move toward target
    Vector2D bestMove = owner.position;
    int bestDistance = INT_MAX;

    for (const auto& move : possibleMoves)
    {
        int dist = std::abs(move.x - targetPosition.x) + std::abs(move.y - targetPosition.y);
        if (dist < bestDistance)
        {
            bestDistance = dist;
            bestMove = move;
        }
    }

    // Move to best position
    owner.position = bestMove;
}

bool AiSpider::canPoisonAttack(Creature& owner)
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
    return (game.d.d100() <= poisonChance);
}

void AiSpider::poisonAttack(Creature& owner, Creature& target)
{
    // Apply poison effect to target if it's the player
    if (&target == game.player.get())
    {
        // Display poison message
        game.appendMessagePart(ORC_PAIR, owner.actorData.name);
        game.appendMessagePart(WHITE_PAIR, " injects you with venom!");
        game.finalizeMessage();

        // TODO: Add poison effect to player
        // This would need a Poison class or status effect implementation
        // For now, just deal 1-3 damage per turn for POISON_DURATION turns

        // Deal initial poison damage
        target.destructible->take_damage(target, game.d.roll(1, 3));

        // Reset cooldown
        poisonCooldown = POISON_COOLDOWN;
    }
}

Vector2D AiSpider::findAmbushPosition(Creature& owner, Vector2D targetPosition)
{
    // Look for positions near walls that are good for ambushing
    std::vector<Vector2D> candidates;

    // Search in a 5x5 area around the current position
    for (int y = -5; y <= 5; y++)
    {
        for (int x = -5; x <= 5; x++)
        {
            Vector2D pos = owner.position + Vector2D{ y, x };

            // Check boundaries
            if (pos.y < 0 || pos.y >= game.map->get_height() ||
                pos.x < 0 || pos.x >= game.map->get_width())
            {
                continue;
            }

            // Check if position is walkable, not occupied, and a good ambush spot
            if (game.map->can_walk(pos) && !game.map->get_actor(pos) && isGoodAmbushSpot(pos))
            {
                // Evaluate position - closer to player is better for ambush
                int distToPlayer = std::abs(pos.x - targetPosition.x) + std::abs(pos.y - targetPosition.y);

                // Only consider positions that are not too far and not too close
                if (distToPlayer >= 3 && distToPlayer <= 8)
                {
                    candidates.push_back(pos);
                }
            }
        }
    }

    // Pick a random good position if available
    if (!candidates.empty())
    {
        int index = game.d.roll(0, candidates.size() - 1);
        return candidates[index];
    }

    // No good ambush position found
    return Vector2D{ -1, -1 };
}

bool AiSpider::isGoodAmbushSpot(Vector2D position)
{
    // Good ambush spots are adjacent to walls and ideally in corners
    int wallCount = 0;

    // Check the 8 surrounding tiles
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            if (x == 0 && y == 0) continue; // Skip center

            Vector2D adj = position + Vector2D{ y, x };

            // Check if this position is a wall
            if (game.map->is_wall(adj))
            {
                wallCount++;
            }
        }
    }

    // Good ambush spots have at least 3 adjacent walls (corner-like)
    return wallCount >= 3;
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

AiWebSpinner::AiWebSpinner()
    : AiSpider(), webCooldown(0)
{
}

void AiWebSpinner::update(Creature& owner)
{
    // Update cooldown timer
    if (webCooldown > 0)
    {
        webCooldown--;
    }

    // Check if we can create a web
    if (webCooldown == 0 && shouldCreateWeb(owner))
    {
        if (tryCreateWeb(owner))
        {
            // Web was created, reset cooldown
            webCooldown = WEB_COOLDOWN;

            // Show message
            game.appendMessagePart(owner.actorData.color, owner.actorData.name);
            game.appendMessagePart(WHITE_PAIR, " spins a sticky web!");
            game.finalizeMessage();

            // Return early - web spinning takes a turn
            return;
        }
    }

    // If we didn't create a web, use regular spider behavior
    AiSpider::update(owner);
}

bool AiWebSpinner::shouldCreateWeb(Creature& owner)
{
    // Web spinners are much more aggressive about creating webs

    // Player doesn't need to be in FOV - spiders can lay trap webs anywhere

    // If player is nearby, high chance to create defensive web
    int distToPlayer = owner.get_tile_distance(game.player->position);
    if (distToPlayer <= 10 && game.d.d100() < 70)
    {
        return true;
    }

    // Even if player is far, occasional web creation for traps (15% chance)
    if (game.d.d100() < 15)
    {
        return true;
    }

    // Check number of existing webs - allow more webs than before
    int webCount = 0;
    for (int y = 0; y < game.map->get_height(); y++)
    {
        for (int x = 0; x < game.map->get_width(); x++)
        {
            // Check if tile is a web
            //if (game.map->get_tile_type(Vector2D{ y, x }) == TileType::WEB)
            //{
            //    webCount++;
            //}
        }
    }

    // Allow up to MAX_WEBS per spider (much higher than before)
    return webCount < MAX_WEBS * 2;
}

bool AiWebSpinner::tryCreateWeb(Creature& owner)
{
    // Determine the web size - bigger webs when player is closer
    int distToPlayer = owner.get_tile_distance(game.player->position);
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

    if (game.map->is_in_fov(owner.position) && distToPlayer < 10)
    {
        // If player can see spider, create web between spider and player
        int dx = game.player->position.x - owner.position.x;
        int dy = game.player->position.y - owner.position.y;

        if (dx != 0) dx = dx / std::abs(dx);
        if (dy != 0) dy = dy / std::abs(dy);

        // Position web to block player's path
        webCenter = owner.position + Vector2D{ dy * 2, dx * 2 };

        // Make sure center is in bounds
        webCenter.x = std::max(0, std::min(webCenter.x, game.map->get_width() - 1));
        webCenter.y = std::max(0, std::min(webCenter.y, game.map->get_height() - 1));
    }
    else
    {
        // If player can't see spider, create web at a nearby location
        webCenter = owner.position;
    }

    // Generate the web pattern - now creating actual Web entities
    generateWebEntities(webCenter, webSize);

    // Dramatic message about web creation
    game.appendMessagePart(DRAGON_PAIR, owner.actorData.name);
    if (webSize >= WEB_MAX_SIZE - 1)
    {
        game.appendMessagePart(WHITE_PAIR, " creates a massive web network!");
    }
    else
    {
        game.appendMessagePart(WHITE_PAIR, " spins a complex web structure!");
    }
    game.finalizeMessage();

    // Mark this spider as having laid a web
    Spider* spider = dynamic_cast<Spider*>(&owner);
    if (spider)
    {
        spider->set_web_laid(true);
    }

    return true;
}

void AiWebSpinner::generateWebPattern(Vector2D center, int size)
{
    // Create a complex web pattern centered at the given position
    // with size determining the radius/complexity

    // Different web patterns
    enum class WebPattern {
        CIRCULAR,
        SPIRAL,
        RADIAL,
        CHAOTIC
    };

    // Choose a random pattern
    WebPattern pattern = static_cast<WebPattern>(game.d.roll(0, 3));

    // Track positions that have been converted to web
    std::vector<Vector2D> webPositions;

    switch (pattern) {
    case WebPattern::CIRCULAR:
        // Create a circular/oval web
        for (int y = -size; y <= size; y++) {
            for (int x = -size; x <= size; x++) {
                
                float normalizedDist = ((float)(x * x) / (size * size)) + ((float)(y * y) / (size * size));

                // Create web with higher density near the edges
                if (normalizedDist <= 1.0f &&
                    (normalizedDist >= 0.7f || game.d.d100() < 40)) {

                    Vector2D pos = center + Vector2D{ y, x };
                    if (isValidWebPosition(pos)) {
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

        for (int i = 0; i < 20 * size; i++) {
            float radius = radiusStep * i;
            if (radius > size) break;

            int x = center.x + (int)(radius * cos(angle));
            int y = center.y + (int)(radius * sin(angle));

            Vector2D pos{ y, x };
            if (isValidWebPosition(pos)) {
                webPositions.push_back(pos);
            }

            angle += 0.5f;

            // Add some random offshoots from the spiral
            if (game.d.d100() < 30) {
                for (int j = 1; j <= 3; j++) {
                    int offX = x + game.d.roll(-1, 1);
                    int offY = y + game.d.roll(-1, 1);

                    Vector2D offPos{ offY, offX };
                    if (isValidWebPosition(offPos)) {
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
        int numSpokes = 6 + game.d.roll(0, 4); // 6-10 spokes

        for (int i = 0; i < numSpokes; i++) {
            float angle = (float)i * (2.0f * 3.14159f / numSpokes);

            for (int dist = 1; dist <= size; dist++) {
                int x = center.x + (int)(dist * cos(angle));
                int y = center.y + (int)(dist * sin(angle));

                Vector2D pos{ y, x };
                if (isValidWebPosition(pos)) {
                    webPositions.push_back(pos);
                }
            }
        }

        // Now create concentric circles connecting the spokes
        for (int radius = 1; radius <= size; radius += 1 + (size / 5)) {
            for (float angle = 0; angle < 2.0f * 3.14159f; angle += 0.2f) {
                int x = center.x + (int)(radius * cos(angle));
                int y = center.y + (int)(radius * sin(angle));

                Vector2D pos{ y, x };
                if (isValidWebPosition(pos)) {
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
        for (int y = -2; y <= 2; y++) {
            for (int x = -2; x <= 2; x++) {
                if (abs(x) + abs(y) <= 3) {
                    Vector2D pos = center + Vector2D{ y, x };
                    if (isValidWebPosition(pos)) {
                        webPositions.push_back(pos);
                    }
                }
            }
        }

        // Then create random strands extending outward
        for (int strand = 0; strand < 8 + game.d.roll(0, 7); strand++) {
            Vector2D strandPos = center;
            int strandLength = game.d.roll(3, size);

            for (int step = 0; step < strandLength; step++) {
                // Random direction but with bias toward continuing current direction
                int dx = game.d.roll(-1, 1);
                int dy = game.d.roll(-1, 1);

                strandPos.x += dx;
                strandPos.y += dy;

                if (isValidWebPosition(strandPos)) {
                    webPositions.push_back(strandPos);
                }
                else {
                    break; // Hit a wall or invalid position
                }

                // Occasionally branch the strand
                if (game.d.d100() < 30) {
                    Vector2D branchPos = strandPos;
                    int branchLength = game.d.roll(2, 4);

                    for (int bStep = 0; bStep < branchLength; bStep++) {
                        branchPos.x += game.d.roll(-1, 1);
                        branchPos.y += game.d.roll(-1, 1);

                        if (isValidWebPosition(branchPos)) {
                            webPositions.push_back(branchPos);
                        }
                        else {
                            break;
                        }
                    }
                }
            }
        }
    }
    break;
    }

    // Place all the web tiles
    for (const auto& pos : webPositions) {
        // Set the tile to web with variable strength
        int webStrength = WEB_STRENGTH;
        if (game.d.d100() < 25) {
            // Some webs are stronger or weaker
            webStrength += game.d.roll(-1, 2);
        }

        // Create the web tile (you'll need to add this method to your Map class)
        /*game.map->set_tile(pos, TileType::WEB, webStrength);*/
    }
}

// Helper method to check if a position is valid for placing a web
bool AiWebSpinner::isValidWebPosition(Vector2D pos)
{
    // Check bounds
    if (pos.y < 0 || pos.y >= game.map->get_height() ||
        pos.x < 0 || pos.x >= game.map->get_width()) {
        return false;
    }

    // Check if the position is walkable
    if (!game.map->can_walk(pos)) {
        return false;
    }

    // Don't place webs on occupied tiles
    if (game.map->get_actor(pos) != nullptr) {
        return false;
    }

    // Check if there's already a web at this position
    for (const auto& obj : game.objects) {
        if (obj && obj->position == pos && obj->actorData.name == "spider web") {
            return false;
        }
    }

    return true;
}

void AiWebSpinner::generateWebEntities(Vector2D center, int size)
{
    // Create a complex web pattern centered at the given position
    // with size determining the radius/complexity

    // Different web patterns
    enum class WebPattern {
        CIRCULAR,
        SPIRAL,
        RADIAL,
        CHAOTIC
    };

    // Choose a random pattern
    WebPattern pattern = static_cast<WebPattern>(game.d.roll(0, 3));

    // Track positions where we want to create webs
    std::vector<Vector2D> webPositions;

    switch (pattern) {
    case WebPattern::CIRCULAR:
        // Create a circular/oval web
        for (int y = -size; y <= size; y++) {
            for (int x = -size; x <= size; x++) {

                float normalizedDist = ((float)(x * x) / (size * size)) + ((float)(y * y) / (size * size));

                // Create web with higher density near the edges
                if (normalizedDist <= 1.0f &&
                    (normalizedDist >= 0.7f || game.d.d100() < 40)) {

                    Vector2D pos = center + Vector2D{ y, x };
                    if (isValidWebPosition(pos)) {
                        webPositions.push_back(pos);
                    }
                }
            }
        }
        break;

        // ... other patterns as before ...
    }

    // Create Web entities at these positions
    for (const auto& pos : webPositions) {
        // Set variable web strength
        int webStrength = WEB_STRENGTH;
        if (game.d.d100() < 25) {
            // Some webs are stronger or weaker
            webStrength += game.d.roll(-1, 2);
        }

        // Create a new Web entity
        auto web = std::make_unique<Web>(pos, webStrength);
        game.objects.emplace_back(std::move(web));
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