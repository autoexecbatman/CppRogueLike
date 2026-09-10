#pragma once
#include <vector>
#include <cassert>

#include <memory>
#include <span>

// Forward declarations
class Creature;
class Item;
class TileFeature;
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
	// Draws whatever is standing on the floor. Templated on the element type so
	// one body serves the trap container and the spell tile container, both of
	// which hold something derived from TileFeature.
	template <typename Feature>
	void render_tile_features(const std::vector<std::unique_ptr<Feature>>& features, const GameContext& ctx) const
	{
		for (const auto& feature : features)
		{
			assert(feature && "a floor container holds a null entry");
			if (!feature->is_destroyed())
			{
				feature->render(ctx);
			}
		}
	}

	void render_decorations(std::span<const std::unique_ptr<Decoration>> decorations, const GameContext& ctx) const;
	void apply_lighting(const GameContext& ctx) const;
	void render_mouse_path_overlay(const GameContext& ctx) const;
	void render_world(const GameContext& ctx) const;
};
