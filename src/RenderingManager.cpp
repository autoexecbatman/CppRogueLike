// RenderingManager.cpp - Handles all rendering and screen management
#include <algorithm>
#include <cassert>
#include <memory>
#include <span>

#include "Actor.h"
#include "Creature.h"
#include "TileFeature.h"
#include "Gui.h"
#include "GameContext.h"
#include "Decoration.h"
#include "Map.h"
#include "Minimap.h"
#include "Renderer.h"
#include "RenderingManager.h"
#include "SpellTile.h"
#include "Trap.h"

void RenderingManager::render(GameContext& ctx) const
{
	render_world(ctx);
}

void RenderingManager::render_world(const GameContext& ctx) const
{
	ctx.map->render(ctx);
	ctx.stairs->render(ctx);

	render_tile_features(*ctx.traps, ctx);
	render_tile_features(*ctx.spellTiles, ctx);

	// Render floor items
	render_items(ctx.floorInventory->items, ctx);

	render_creatures(*ctx.creatures, ctx);
	ctx.player()->render(ctx);

	if (ctx.decorations)
	{
		render_decorations(*ctx.decorations, ctx);
	}

	apply_lighting(ctx);
	render_mouse_path_overlay(ctx);

	if (ctx.minimap)
	{
		ctx.minimap->render(ctx);
	}
}

void RenderingManager::render_creatures(std::span<const std::unique_ptr<Creature>> creatures, const GameContext& ctx) const
{
	for (const auto& creature : creatures)
	{
		assert(creature && "creatures holds a null entry");
		creature->render(ctx);
	}
}

void RenderingManager::render_items(std::span<const std::unique_ptr<Item>> items, const GameContext& ctx) const
{
	for (const auto& item : items)
	{
		assert(item && "items holds a null entry");
		item->render(ctx);
	}
}

void RenderingManager::safe_screen_clear()
{
	// No-op: Renderer handles frame clearing via ClearBackground(BLACK) in begin_frame()
}

void RenderingManager::force_screen_refresh() const
{
	// No-op: frame-based rendering handles this via Renderer::end_frame()
}

void RenderingManager::restore_game_display() const
{
	// Force screen refresh for display restoration
	force_screen_refresh();
}

void RenderingManager::restore_screen(GameContext& ctx) const
{
	assert(ctx.gui != nullptr);

	render(ctx);
	ctx.gui->gui_render(ctx);
}

void RenderingManager::apply_lighting(const GameContext& ctx) const
{
	if (!ctx.renderer || !ctx.map || !ctx.player())
	{
		return;
	}

	Renderer& renderer = *ctx.renderer;
	const int tileSize = renderer.get_tile_size();
	const int cameraX = renderer.get_camera_x();
	const int cameraY = renderer.get_camera_y();

	static constexpr float torchRadiusTiles = 6.5f;
	// The light is a multiply over sprites that are already painted in colour, so
	// whatever it takes out of a channel is taken out of the art. The old inner
	// colour held blue at 55 percent of red and applied that at full strength on the
	// player's own tile, which pulled a yellow cast over every lit pixel - measured
	// at blue/red 0.48 to 0.55 right across the radius. Near the flame the light is
	// bright enough to read as white; the warmth belongs at the edge, where the
	// falloff now runs out through amber instead of straight to black.
	static constexpr Color torchInner = { 255, 250, 240, 255 };
	static constexpr Color torchOuter = { 34, 18, 6, 255 };
	// Remembered ground is dimmer and a touch cooler than lit ground, so the two
	// read apart by brightness rather than by hue.
	static constexpr Color exploredMemoryLight = { 150, 152, 158, 255 };

	auto lerp_channel = [](unsigned char fromChannel, unsigned char toChannel, float fraction) -> unsigned char
	{
		return static_cast<unsigned char>(
			static_cast<float>(fromChannel) + fraction * (static_cast<float>(toChannel) - static_cast<float>(fromChannel)));
	};

	renderer.begin_light_mask();

	for (int tileY = 0; tileY < ctx.map->get_height(); ++tileY)
	{
		for (int tileX = 0; tileX < ctx.map->get_width(); ++tileX)
		{
			Vector2D tilePos{ tileX, tileY };
			if (!ctx.map->is_in_fov(tilePos))
			{
				if (ctx.map->is_explored(tilePos))
				{
					int screenX = tileX * tileSize - cameraX;
					int screenY = tileY * tileSize - cameraY;
					renderer.add_light_quad(screenX, screenY, tileSize, exploredMemoryLight);
				}
				continue;
			}

			float distanceTiles = static_cast<float>(tilePos.distance_to(ctx.player()->position));
			float falloff = std::min(distanceTiles / torchRadiusTiles, 1.0f);

			Color litColor{
				lerp_channel(torchInner.r, torchOuter.r, falloff),
				lerp_channel(torchInner.g, torchOuter.g, falloff),
				lerp_channel(torchInner.b, torchOuter.b, falloff),
				255
			};

			int screenX = tileX * tileSize - cameraX;
			int screenY = tileY * tileSize - cameraY;
			renderer.add_light_quad(screenX, screenY, tileSize, litColor);
		}
	}

	renderer.apply_light_mask();
}

void RenderingManager::render_decorations(
	std::span<const std::unique_ptr<Decoration>> decorations,
	const GameContext& ctx) const
{
	if (!ctx.renderer || !ctx.map)
	{
		return;
	}
	for (const auto& decor : decorations)
	{
		if (!decor || decor->isBroken)
		{
			continue;
		}
		if (!ctx.map->is_in_fov(decor->position))
		{
			continue;
		}
		ctx.renderer->draw_tile_static(decor->position, decor->tile, Color{ 255, 255, 255, 255 });
	}
}

void RenderingManager::render_mouse_path_overlay(const GameContext& ctx) const
{
	if (!ctx.renderer || !ctx.mousePathOverlay || ctx.mousePathOverlay->empty())
	{
		return;
	}

	const int tileSize = ctx.renderer->get_tile_size();
	const int cam_x = ctx.renderer->get_camera_x();
	const int cam_y = ctx.renderer->get_camera_y();

	const int dotRadius = std::max(2, tileSize / 8);
	const int destRadius = std::max(3, tileSize / 5);
	const size_t count = ctx.mousePathOverlay->size();
	const size_t lastIdx = count - 1;

	// Pre-compute screen centres
	auto screen_centre = [&](size_t i) -> Vector2D
	{
		const Vector2D& p = (*ctx.mousePathOverlay)[i];
		return { p.x * tileSize - cam_x + tileSize / 2,
			p.y * tileSize - cam_y + tileSize / 2 };
	};

	// Pass 1 — lines between consecutive nodes, alpha fades toward destination
	for (size_t i = 0; i + 1 < count; ++i)
	{
		Vector2D a = screen_centre(i);
		Vector2D b = screen_centre(i + 1);

		float t = static_cast<float>(i) / static_cast<float>(lastIdx);
		unsigned char alpha = static_cast<unsigned char>(55 + static_cast<int>(80.0f * t));
		Color lineColor = { 220, 220, 220, alpha };

		DrawLineEx(
			{ static_cast<float>(a.x), static_cast<float>(a.y) },
			{ static_cast<float>(b.x), static_cast<float>(b.y) },
			2.0f,
			lineColor);
	}

	// Pass 2 — dots on top of lines, alpha fades toward destination
	for (size_t i = 0; i < count; ++i)
	{
		Vector2D sc = screen_centre(i);

		float t = static_cast<float>(i) / static_cast<float>(lastIdx);
		unsigned char alpha = static_cast<unsigned char>(80 + static_cast<int>(120.0f * t));

		if (i == lastIdx)
		{
			// Destination: bright ring
			Color destFill = { 255, 255, 255, static_cast<unsigned char>(alpha) };
			Color destRing = { 255, 255, 255, 200 };
			DrawCircle(sc.x, sc.y, static_cast<float>(destRadius), destFill);
			DrawCircleLines(sc.x, sc.y, static_cast<float>(destRadius + 2), destRing);
		}
		else
		{
			Color nodeColor = { 210, 210, 210, alpha };
			DrawCircle(sc.x, sc.y, static_cast<float>(dotRadius), nodeColor);
		}
	}
}
