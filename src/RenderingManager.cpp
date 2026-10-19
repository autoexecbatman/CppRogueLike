// RenderingManager.cpp - Handles all rendering and screen management
#include <algorithm>
#include <cassert>
#include <memory>
#include <span>

#include "Actor.h"
#include "CostOverlay.h"
#include "Creature.h"
#include "Decoration.h"
#include "GameContext.h"
#include "Gui.h"
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
	assert(ctx.minimap && "RenderingManager::render_world: called without a minimap");
	assert(ctx.costOverlay && "RenderingManager::render_world: called without a costOverlay");
	assert(ctx.traps && "RenderingManager::render_world: called without traps");
	assert(ctx.stairs && "RenderingManager::render_world: called without stairs");
	assert(ctx.spellTiles && "RenderingManager::render_world: called without spellTiles");
	assert(ctx.creatures && "RenderingManager::render_world: called without creatures");
	assert(ctx.floorInventory && "RenderingManager::render_world: called without a floorInventory");
	assert(ctx.map && "RenderingManager::render_world: called without a map");

	ctx.map->render(ctx);
	ctx.stairs->render(ctx);

	render_tile_features(*ctx.traps, ctx);
	render_tile_features(*ctx.spellTiles, ctx);

	// Render floor items
	render_items(ctx.floorInventory->items, ctx);

	render_creatures(*ctx.creatures, ctx);
	ctx.player()->render(ctx);

	assert(ctx.decorations && "RenderingManager::render_world: called without decorations");
	render_decorations(*ctx.decorations, ctx);

	apply_lighting(ctx);
	render_mouse_path_overlay(ctx);

	// After the lighting, so a cost is readable in the dark parts of the map too.
	ctx.costOverlay->render(ctx);

	ctx.minimap->render(ctx);
}

void RenderingManager::render_creatures(std::span<const std::unique_ptr<Creature>> creatures, const GameContext& ctx) const
{
	for (const auto& creature : creatures)
	{
		assert(creature && "render_creatures: creatures list holds a null entry");
		creature->render(ctx);
	}
}

void RenderingManager::render_items(std::span<const std::unique_ptr<Item>> items, const GameContext& ctx) const
{
	for (const auto& item : items)
	{
		assert(item && "render_items: items list holds a null entry");
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
	assert(ctx.gui != nullptr && "RenderingManager::restore_screen: called without a gui");

	render(ctx);
	ctx.gui->gui_render(ctx);
}

void RenderingManager::apply_lighting(const GameContext& ctx) const
{
	assert(ctx.map && "RenderingManager::apply_lighting: called without a map");
	if (!ctx.renderer || !ctx.player())
	{
		return;
	}

	Renderer& renderer = *ctx.renderer;
	const int tileSize = renderer.get_tile_size();
	const int cameraX = renderer.get_camera_x();
	const int cameraY = renderer.get_camera_y();

	static constexpr float torchRadiusTiles = 6.5f;
	// The light multiplies painted sprites, so a warm inner colour casts over every lit pixel;
	// near white at the flame, with the warmth at the edge as the falloff runs through amber.
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
	assert(ctx.map && "RenderingManager::render_decorations: called without a map");
	if (!ctx.renderer)
	{
		return;
	}
	for (const auto& decor : decorations)
	{
		assert(decor && "render_decorations: the decoration list holds a null entry");

		if (decor->isBroken)
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
	assert(ctx.mousePathOverlay && "RenderingManager::render_mouse_path_overlay: called without a mousePathOverlay");

	if (!ctx.renderer || ctx.mousePathOverlay->empty())
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
	auto screen_centre = [&](size_t nodeIndex) -> Vector2D
	{
		const Vector2D& node = (*ctx.mousePathOverlay)[nodeIndex];
		return { node.x * tileSize - cam_x + tileSize / 2,
			node.y * tileSize - cam_y + tileSize / 2 };
	};

	// Pass 1 - lines between consecutive nodes, alpha fades toward destination
	for (size_t nodeIndex = 0; nodeIndex + 1 < count; ++nodeIndex)
	{
		Vector2D lineStart = screen_centre(nodeIndex);
		Vector2D lineEnd = screen_centre(nodeIndex + 1);

		float progress = static_cast<float>(nodeIndex) / static_cast<float>(lastIdx);
		unsigned char alpha = static_cast<unsigned char>(55 + static_cast<int>(80.0f * progress));
		Color lineColor = { 220, 220, 220, alpha };

		DrawLineEx(
			{ static_cast<float>(lineStart.x), static_cast<float>(lineStart.y) },
			{ static_cast<float>(lineEnd.x), static_cast<float>(lineEnd.y) },
			2.0f,
			lineColor);
	}

	// Pass 2 - dots on top of lines, alpha fades toward destination
	for (size_t nodeIndex = 0; nodeIndex < count; ++nodeIndex)
	{
		Vector2D centre = screen_centre(nodeIndex);

		float progress = static_cast<float>(nodeIndex) / static_cast<float>(lastIdx);
		unsigned char alpha = static_cast<unsigned char>(80 + static_cast<int>(120.0f * progress));

		if (nodeIndex == lastIdx)
		{
			// Destination: bright ring
			Color destFill = { 255, 255, 255, static_cast<unsigned char>(alpha) };
			Color destRing = { 255, 255, 255, 200 };
			DrawCircle(centre.x, centre.y, static_cast<float>(destRadius), destFill);
			DrawCircleLines(centre.x, centre.y, static_cast<float>(destRadius + 2), destRing);
		}
		else
		{
			Color nodeColor = { 210, 210, 210, alpha };
			DrawCircle(centre.x, centre.y, static_cast<float>(dotRadius), nodeColor);
		}
	}
}
