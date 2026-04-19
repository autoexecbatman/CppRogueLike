#pragma once

#include <memory>
#include <span>

// Forward declarations
class Creature;
class Item;
class Object;
struct Decoration;
struct GameContext;

// - Handles all rendering and screen management
class RenderingManager
{
public:
	// Core rendering methods
	void render(GameContext& ctx) const;

	void render_creatures(std::span<const std::unique_ptr<Creature>> creatures, const GameContext& ctx) const;
	void render_items(std::span<const std::unique_ptr<Item>> items, const GameContext& ctx) const;

	// Screen management
	void safe_screen_clear();
	void force_screen_refresh() const;
	void restore_game_display() const;
	void restore_screen(GameContext& ctx) const;

private:
	// Helper methods
	void render_objects(std::span<const std::unique_ptr<Object>> objects, const GameContext& ctx) const;
	void render_decorations(std::span<const std::unique_ptr<Decoration>> decorations, const GameContext& ctx) const;
	void apply_lighting(const GameContext& ctx) const;
	void render_mouse_path_overlay(const GameContext& ctx) const;
	void render_world(const GameContext& ctx) const;
};
