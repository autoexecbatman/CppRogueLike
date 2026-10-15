// file: CostOverlay.cpp
#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>
#include <string>

#include <raylib.h>

#include "CostOverlay.h"
#include "GameContext.h"
#include "Map.h"
#include "Renderer.h"
#include "Vector2D.h"

namespace
{

// The cost the flood leaves on a tile it never reached.
constexpr int UNREACHED_COST = std::numeric_limits<int>::max();

unsigned char blend_channel(unsigned char nearValue, unsigned char farValue, int cost, int farthestCost)
{
	return static_cast<unsigned char>(nearValue + (farValue - nearValue) * cost / farthestCost);
}

} // namespace

std::optional<std::string> dijkstra_cost_label(int cost)
{
	if (cost == UNREACHED_COST)
	{
		return std::nullopt;
	}
	return std::to_string(cost);
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

	const Map& map = *ctx.map;
	const Renderer& renderer = *ctx.renderer;

	// The tint is relative to the farthest tile reached, so find it before drawing any.
	int farthestCost = 0;
	for (int row = 0; row < map.get_height(); ++row)
	{
		for (int col = 0; col < map.get_width(); ++col)
		{
			const int cost = map.get_dijkstra_cost(Vector2D{ col, row });
			if (cost != UNREACHED_COST)
			{
				farthestCost = std::max(farthestCost, cost);
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

			const int cost = map.get_dijkstra_cost(Vector2D{ col, row });
			const std::optional<std::string> label = dijkstra_cost_label(cost);
			if (!label)
			{
				continue;
			}

			DrawRectangle(screenX, screenY, tileSize, tileSize, dijkstra_cost_tint(cost, farthestCost));

			// Centred on the tile, so a three-digit cost does not run into its neighbour.
			const int labelX = screenX + (tileSize - renderer.measure_text(*label)) / 2;
			const int labelY = screenY + (tileSize - renderer.get_font_size()) / 2;
			renderer.draw_text_color(Vector2D{ labelX, labelY }, *label, WHITE);
		}
	}
}
