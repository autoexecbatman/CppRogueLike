#include "AiMonsterRanged.h"
#include "../Game.h"
#include "../Utils/Vector2D.h"
#include <curses.h>

AiMonsterRanged::AiMonsterRanged() : AiMonster() {}

void AiMonsterRanged::update(Creature& owner)
{
    if (owner.destructible->is_dead()) {
        return;
    }

    if (game.map->is_in_fov(owner.position)) {
        // Move towards the player if we can see them
        moveCount = TRACKING_TURNS;
    }
    else {
        moveCount--;
    }

    if (moveCount > 0) {
        moveOrAttack(owner, game.player->position);
    }
}

void AiMonsterRanged::moveOrAttack(Creature& owner, Vector2D targetPosition)
{
    int distance = owner.get_tile_distance(targetPosition);

    // If at optimal range and have line of sight, try a ranged attack
    if (distance <= maxRangeDistance && distance >= 2 && game.map->has_los(owner.position, targetPosition)) {
        if (tryRangedAttack(owner, targetPosition)) {
            return; // Attack succeeded, end turn
        }
    }

    // If too close, try to back away
    if (distance < optimalDistance) {
        // Calculate direction away from player
        Vector2D moveDir = owner.position - targetPosition;

        // Normalize and scale
        int maxComponent = std::max(std::abs(moveDir.x), std::abs(moveDir.y));
        if (maxComponent > 0) {
            moveDir.x = moveDir.x / maxComponent;
            moveDir.y = moveDir.y / maxComponent;
        }

        // Try to move away in this direction
        Vector2D newPos = owner.position + moveDir;
        if (game.map->can_walk(newPos) && !game.map->get_actor(newPos)) {
            owner.position = newPos;
            return;
        }
    }

    // If too far or couldn't back up, use regular pathfinding to approach
    if (distance > maxRangeDistance || distance < optimalDistance) {
        AiMonster::moveOrAttack(owner, targetPosition);
    }
}

bool AiMonsterRanged::tryRangedAttack(Creature& owner, Vector2D targetPos)
{
    // Check if there's a clear line of sight
    if (!game.map->has_los(owner.position, targetPos)) {
        return false;
    }

    // Animate the projectile
    char projChar = (owner.actorData.name == "archer") ? '/' : '*';
    animateProjectile(owner.position, targetPos, projChar);

    // Perform the attack
    owner.attacker->attack(owner, *game.player);
    return true;
}

void AiMonsterRanged::animateProjectile(Vector2D from, Vector2D to, char projectileChar)
{
    // Use Bresenham's line algorithm for the projectile path
    int x0 = from.x;
    int y0 = from.y;
    int x1 = to.x;
    int y1 = to.y;

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

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

        // Skip start point
        if (x0 == from.x && y0 == from.y) continue;

        // Stop at end point or walls
        if ((x0 == to.x && y0 == to.y) || !game.map->can_walk(Vector2D{ y0, x0 })) break;

        // Draw projectile and refresh
        mvaddch(y0, x0, projectileChar);
        refresh();
        napms(30); // Short delay

        // Erase the projectile by redrawing the original tile
        if (game.map->is_in_fov(Vector2D{ y0, x0 })) {
            if (game.map->can_walk(Vector2D{ y0, x0 })) {
                mvaddch(y0, x0, '.');
            }
            else {
                mvaddch(y0, x0, '#');
            }
        }
        else {
            mvaddch(y0, x0, ' ');
        }
    }
}

void AiMonsterRanged::load(const json& j)
{
    AiMonster::load(j);
    // Additional ranged AI properties can be loaded here
    if (j.contains("maxRangeDistance")) {
        maxRangeDistance = j.at("maxRangeDistance").get<int>();
    }
    if (j.contains("optimalDistance")) {
        optimalDistance = j.at("optimalDistance").get<int>();
    }
}

void AiMonsterRanged::save(json& j)
{
    AiMonster::save(j);
    // Additional ranged AI properties
    j["maxRangeDistance"] = maxRangeDistance;
    j["optimalDistance"] = optimalDistance;
}
