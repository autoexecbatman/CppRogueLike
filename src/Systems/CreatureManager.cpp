// CreatureManager.cpp - Handles all creature lifecycle and management
#include <algorithm>
#include <climits>

#include "CreatureManager.h"
#include "../Actor/Actor.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "../Game.h"
#include "../Core/GameContext.h"

void CreatureManager::update_creatures(std::span<std::unique_ptr<Creature>> creatures, GameContext& ctx)
{
    for (const auto& creature : creatures)
    {
        if (creature)
        {
            creature->update(ctx);
        }
    }
}

void CreatureManager::cleanup_dead_creatures(std::vector<std::unique_ptr<Creature>>& creatures)
{
    // Remove dead creatures from the game
    // This is called at safe points to avoid dangling references during combat
    std::erase_if(creatures, [](const auto& creature) 
    {
        return creature && creature->destructible && creature->destructible->is_dead();
    });
}

void CreatureManager::spawn_creatures(
    std::vector<std::unique_ptr<Creature>>& creatures,
    std::span<const Vector2D> rooms,
    Map& map,
    RandomDice& dice,
    int game_time,
    GameContext& ctx,
    int max_creatures,
    int spawn_rate
)
{
    // INCREASED SPAWN RATE - add a new monster every spawn_rate turns (default 2)
    if (game_time % spawn_rate == 0)
    {
        if (can_spawn_creature(creatures, max_creatures))
        {
            Vector2D spawnPos = find_spawn_position(rooms, map, dice, ctx);
            map.add_monster(spawnPos, ctx);
        }
    }
}

Creature* CreatureManager::get_closest_monster(
    std::span<const std::unique_ptr<Creature>> creatures,
    Vector2D fromPosition,
    double inRange
) const noexcept
{
    Creature* closestMonster = nullptr;
    int bestDistance = INT_MAX;

    for (const auto& actor : creatures)
    {
        if (!actor->destructible->is_dead())
        {
            const int distance = actor->get_tile_distance(fromPosition);
            if (distance < bestDistance && (distance <= inRange || inRange == 0.0f))
            {
                bestDistance = distance;
                closestMonster = actor.get();
            }
        }
    }

    return closestMonster;
}

Creature* CreatureManager::get_actor_at_position(
    std::span<const std::unique_ptr<Creature>> creatures,
    Vector2D pos
) const noexcept
{
    for (const auto& actor : creatures)
    {
        if (actor->position == pos)
        {
            return actor.get();
        }
    }
    return nullptr;
}

bool CreatureManager::can_spawn_creature(
    std::span<const std::unique_ptr<Creature>> creatures,
    int max_creatures
) const noexcept
{
    return creatures.size() < static_cast<size_t>(max_creatures);
}

Vector2D CreatureManager::find_spawn_position(
    std::span<const Vector2D> rooms,
    Map& map,
    RandomDice& dice,
    GameContext& ctx
)
{
    if (rooms.empty())
    {
        throw std::runtime_error("rooms vector is empty!");
    }

    // Roll a random index as the size of the rooms vector
    int index = dice.roll(0, static_cast<int>(rooms.size()) - 1);

    // Make the index even
    index = index % 2 == 0 ? index : index - 1;

    // Get the room begin and end
    const Vector2D roomBegin = rooms[index];
    const Vector2D roomEnd = rooms[index + 1];

    // Get a random position in the room
    Vector2D pos = Vector2D{ dice.roll(roomBegin.y, roomEnd.y), dice.roll(roomBegin.x, roomEnd.x) };

    // If pos is at wall roll again
    while (!map.can_walk(pos, ctx))
    {
        pos.x = dice.roll(roomBegin.x, roomEnd.x);
        pos.y = dice.roll(roomBegin.y, roomEnd.y);
    }

    return pos;
}
