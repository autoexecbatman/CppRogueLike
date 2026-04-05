// RenderingManager.cpp - Handles all rendering and screen management
#include <algorithm>
#include <memory>
#include <span>

#include "../Actor/Actor.h"
#include "../Actor/InventoryData.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Gui/Gui.h"
#include "../Map/Decoration.h"
#include "../Map/Map.h"
#include "../Map/Minimap.h"
#include "../Renderer/Renderer.h"
#include "RenderingManager.h"

void RenderingManager::render(GameContext& ctx) const
{
	render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventoryData, *ctx.creatures, *ctx.player, ctx);
}

void RenderingManager::render_world(
	const Map& map,
	const Stairs& stairs,
	std::span<const std::unique_ptr<Object>> objects,
	const InventoryData& inventory_data,
	std::span<const std::unique_ptr<Creature>> creatures,
	const Player& player,
	const GameContext& ctx) const
{
	map.render(ctx);
	stairs.render(ctx);

	render_objects(objects, ctx);

	// Render floor items
	render_items(inventory_data.items, ctx);

	render_creatures(creatures, ctx);
	player.render(ctx);

	if (ctx.decorations)
	{
		render_decorations(*ctx.decorations, ctx);
	}

	apply_lighting(player, ctx);
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
		if (creature)
		{
			creature->render(ctx);
		}
	}
}

void RenderingManager::render_items(std::span<const std::unique_ptr<Item>> items, const GameContext& ctx) const
{
	for (const auto& item : items)
	{
		if (item)
		{
			item->render(ctx);
		}
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
	render(ctx);
	ctx.gui->gui_render(ctx);
}

void RenderingManager::apply_lighting(const Player& player, const GameContext& ctx) const
{
	if (!ctx.renderer)
		return;

	Renderer& r = *ctx.renderer;
	int tileSize = r.get_tile_size();
	int cam_x = r.get_camera_x();
	int cam_y = r.get_camera_y();

	Vector2D screenPos{
		player.position.x * tileSize - cam_x + tileSize / 2,
		player.position.y * tileSize - cam_y + tileSize / 2
	};

	static constexpr float torchRadiusTiles = 6.5f;
	float radius = torchRadiusTiles * static_cast<float>(tileSize);

	// Warm torch inner, fade to darkness
	static constexpr Color torchInner = { 255, 210, 140, 255 };
	static constexpr Color torchOuter = { 0, 0, 0, 255 };

	r.begin_light_mask();
	r.add_light_source(screenPos, radius, torchInner, torchOuter);
	r.apply_light_mask();
}

void RenderingManager::render_objects(std::span<const std::unique_ptr<Object>> objects, const GameContext& ctx) const
{
	// Render any objects (like webs)
	for (const auto& obj : objects)
	{
		if (obj)
		{
			obj->render(ctx);
		}
	}
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
