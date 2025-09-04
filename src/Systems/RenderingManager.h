// RenderingManager.h - Handles all rendering and screen management

#ifndef RENDERINGMANAGER_H
#define RENDERINGMANAGER_H

#pragma once

#include <span>
#include <memory>
#include <vector>

// Forward declarations
class Creature;
class Item;
class Map;
class Player;
class Stairs;
class Object;
class Container;
class Gui;

class RenderingManager
{
public:
    RenderingManager() = default;
    ~RenderingManager() = default;

    // Core rendering methods
    void render_world(
        const Map& map,
        const Stairs& stairs,
        std::span<const std::unique_ptr<Object>> objects,
        const Container& container,
        std::span<const std::unique_ptr<Creature>> creatures,
        const Player& player
    ) const;

    void render_creatures(std::span<const std::unique_ptr<Creature>> creatures) const;
    void render_items(std::span<const std::unique_ptr<Item>> items) const;

    // Screen management
    void safe_screen_clear();
    void force_screen_refresh() const;
    void restore_game_display() const;

private:
    // Helper methods
    void render_objects(std::span<const std::unique_ptr<Object>> objects) const;
};

#endif // RENDERINGMANAGER_H
