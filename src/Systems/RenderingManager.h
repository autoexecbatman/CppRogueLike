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
        Map& map,
        Stairs& stairs,
        std::span<std::unique_ptr<Object>> objects,
        Container& container,
        std::span<std::unique_ptr<Creature>> creatures,
        Player& player
    );

    void render_creatures(std::span<std::unique_ptr<Creature>> creatures) const;
    void render_items(std::span<std::unique_ptr<Item>> items) const;

    // Screen management
    void safe_screen_clear();
    void force_screen_refresh() const;
    void restore_game_display(Map& map, Gui& gui);

private:
    // Helper methods
    void render_objects(std::span<std::unique_ptr<Object>> objects) const;
};

#endif // RENDERINGMANAGER_H
