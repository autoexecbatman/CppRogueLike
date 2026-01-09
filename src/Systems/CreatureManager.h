// CreatureManager.h - Handles all creature lifecycle and management

#ifndef CREATUREMANAGER_H
#define CREATUREMANAGER_H

#pragma once

#include <memory>
#include <vector>
#include <span>

// Forward declarations
class Creature;
class Map;
class RandomDice;
struct Vector2D;
struct GameContext;

class CreatureManager
{
public:
    CreatureManager() = default;
    ~CreatureManager() = default;

    // Creature lifecycle
    void update_creatures(std::span<std::unique_ptr<Creature>> creatures);
    void cleanup_dead_creatures(std::vector<std::unique_ptr<Creature>>& creatures);
    
    // Spawning
    void spawn_creatures(
        std::vector<std::unique_ptr<Creature>>& creatures,
        std::span<const Vector2D> rooms,
        Map& map,
        RandomDice& dice,
        int game_time,
        GameContext& ctx,
        int max_creatures = 10,
        int spawn_rate = 2
    );
    
    // Queries
    Creature* get_closest_monster(
        std::span<const std::unique_ptr<Creature>> creatures,
        Vector2D fromPosition,
        double inRange
    ) const noexcept;
    
    Creature* get_actor_at_position(
        std::span<const std::unique_ptr<Creature>> creatures,
        Vector2D pos
    ) const noexcept;

    // Template utility for moving creatures to back
    template<typename T>
    void send_to_back(std::vector<std::unique_ptr<Creature>>& creatures, T& actor)
    {
        auto actorIsInVector = [&actor](const auto& a) noexcept { return a.get() == &actor; };
        auto it = std::find_if(creatures.begin(), creatures.end(), actorIsInVector);
        const auto distance = std::distance(creatures.begin(), it);
        for (auto i = distance; i > 0; i--)
        {
            std::swap(creatures[i - 1], creatures[i]);
        }
    }

private:
    // Helper methods
    bool can_spawn_creature(
        std::span<const std::unique_ptr<Creature>> creatures,
        int max_creatures
    ) const noexcept;
    
    Vector2D find_spawn_position(
        std::span<const Vector2D> rooms,
        Map& map,
        RandomDice& dice
    );
};

#endif // CREATUREMANAGER_H
