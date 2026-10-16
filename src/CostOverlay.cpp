// file: CostOverlay.cpp
#include <algorithm>
#include <cassert>
#include <optional>
#include <string>

#include <raylib.h>

#include "CostOverlay.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Renderer.h"
#include "Vector2D.h"

namespace
{

unsigned char blend_channel(unsigned char nearValue, unsigned char farValue, int cost, int farthestCost)
{
	return static_cast<unsigned char>(nearValue + (farValue - nearValue) * cost / farthestCost);
}

} // namespace

std::optional<std::string> dijkstra_cost_label(std::optional<int> cost)
{
	if (!cost)
	{
		return std::nullopt;
	}
	return std::to_string(*cost);
}

Color dijkstra_cost_tint(int cost, int farthestCost)
{
	assert(cost >= 0 && cost <= farthestCost && "dijkstra_cost_tint: cost lies outside the reached range");

	// Only the player's tile was reached, so there is no far end to blend toward.
	if (farthestCost == 0)
	{
		return COST_NEAR_TINT;
	}
	return Color{
		blend_channel(COST_NEAR_TINT.r, COST_FAR_TINT.r, cost, farthestCost),
		blend_channel(COST_NEAR_TINT.g, COST_FAR_TINT.g, cost, farthestCost),
		blend_channel(COST_NEAR_TINT.b, COST_FAR_TINT.b, cost, farthestCost),
		blend_channel(COST_NEAR_TINT.a, COST_FAR_TINT.a, cost, farthestCost),
	};
}

void CostOverlay::toggle() noexcept
{
	visible = !visible;
}

bool CostOverlay::is_visible() const noexcept
{
	return visible;
}

void CostOverlay::render(const GameContext& ctx) const
{
	if (!visible)
	{
		return;
	}
	assert(ctx.map && "CostOverlay::render called without a map");
	assert(ctx.renderer && "CostOverlay::render called without a renderer");
	assert(ctx.pathfinder && "CostOverlay::render called without a pathfinder");
	assert(ctx.tileConfig && "CostOverlay::render called without a tile configuration");

	const Map& map = *ctx.map;
	const Renderer& renderer = *ctx.renderer;

	// The map a creature that moves as the player does would descend toward the player.
	const Vector2D goal = ctx.player()->position;
	const Pathfinding::Mobility mobility = Pathfinding::mobility_of(*ctx.player(), *ctx.tileConfig);
	const auto cost_at = [&](int col, int row)
	{
		return ctx.pathfinder->cost_to(goal, Vector2D{ col, row }, mobility, ctx);
	};

	// The tint is relative to the farthest tile reached, so find it before drawing any.
	int farthestCost = 0;
	for (int row = 0; row < map.get_height(); ++row)
	{
		for (int col = 0; col < map.get_width(); ++col)
		{
			const std::optional<int> cost = cost_at(col, row);
			if (cost)
			{
				farthestCost = std::max(farthestCost, *cost);
			}
		}
	}

	const int tileSize = renderer.get_tile_size();
	const int cameraX = renderer.get_camera_x();
	const int cameraY = renderer.get_camera_y();
	const int screenWidth = renderer.get_screen_width();
	const int screenHeight = renderer.get_screen_height();

	for (int row = 0; row < map.get_height(); ++row)
	{
		for (int col = 0; col < map.get_width(); ++col)
		{
			const int screenX = col * tileSize - cameraX;
			const int screenY = row * tileSize - cameraY;

			// A tile wholly off the screen draws nothing anyone can see.
			if (screenX + tileSize <= 0 || screenY + tileSize <= 0 || screenX >= screenWidth || screenY >= screenHeight)
			{
				continue;
			}

			const std::optional<int> cost = cost_at(col, row);
			const std::optional<std::string> label = dijkstra_cost_label(cost);
			if (!label)
			{
				continue;
			}

			DrawRectangle(screenX, screenY, tileSize, tileSize, dijkstra_cost_tint(*cost, farthestCost));

			// Centred on the tile, so a three-digit cost does not run into its neighbour.
			const int labelX = screenX + (tileSize - renderer.measure_text(*label)) / 2;
			const int labelY = screenY + (tileSize - renderer.get_font_size()) / 2;
			renderer.draw_text_color(Vector2D{ labelX, labelY }, *label, WHITE);
		}
	}
}
