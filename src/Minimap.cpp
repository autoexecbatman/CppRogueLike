// file: Minimap.cpp
#include <algorithm>
#include <cassert>

#include <raylib.h>

#include "GameContext.h"
#include "Map.h"
#include "Minimap.h"
#include "Player.h"
#include "Renderer.h"
#include "Stairs.h"
#include "TileConfig.h"

void Minimap::toggle() noexcept
{
	visible = !visible;
}

bool Minimap::is_visible() const noexcept
{
	return visible;
}

void Minimap::render(const GameContext& ctx) const
{
	assert(ctx.tileConfig && "Minimap::render: called without a tileConfig");
	assert(ctx.map && "Minimap::render: called without a map");
	if (!visible || !ctx.renderer || !ctx.player())
	{
		return;
	}

	const Map& map = *ctx.map;
	const Renderer& renderer = *ctx.renderer;

	int mapW = map.get_width();
	int mapH = map.get_height();
	int screenW = renderer.get_screen_width();
	int screenH = renderer.get_screen_height();

	// The largest whole number of pixels a tile can take inside the overlay's share, so every cell matches.
	const int tilePx = std::max(1, std::min(screenW * MAX_WIDTH_PERCENT / 100 / mapW, screenH * MAX_HEIGHT_PERCENT / 100 / mapH));

	int panelW = mapW * tilePx;
	int panelH = mapH * tilePx;
	int originX = screenW - panelW - PADDING;
	int originY = PADDING;

	// Opaque, with an edge: a translucent backdrop vanishes against unexplored map and leaves no bounds.
	DrawRectangle(originX - BORDER, originY - BORDER, panelW + 2 * BORDER, panelH + 2 * BORDER, Color{ 20, 12, 28, 255 });
	DrawRectangleLines(originX - BORDER, originY - BORDER, panelW + 2 * BORDER, panelH + 2 * BORDER, Color{ 120, 120, 150, 255 });

	for (int row = 0; row < mapH; ++row)
	{
		for (int col = 0; col < mapW; ++col)
		{
			Vector2D pos{ col, row };
			if (!map.is_explored(pos))
			{
				continue;
			}

			bool inFov = map.is_in_fov(pos);
			Color cellColor{};

			const TileDefinition& definition = ctx.tileConfig->get_tile_definition(map.get_tile_type(pos));
			const TileColor& authored = inFov ? definition.minimapVisible : definition.minimapRemembered;
			cellColor = Color{
				static_cast<unsigned char>(authored.red),
				static_cast<unsigned char>(authored.green),
				static_cast<unsigned char>(authored.blue),
				static_cast<unsigned char>(authored.alpha)
			};

			DrawRectangle(originX + col * tilePx, originY + row * tilePx, tilePx, tilePx, cellColor);
		}
	}

	assert(ctx.stairs && "Minimap::render: called without stairs");
	Vector2D stairsPos = ctx.stairs->position;
	if (map.is_explored(stairsPos))
	{
		DrawRectangle(
			originX + stairsPos.x * tilePx - 1,
			originY + stairsPos.y * tilePx - 1,
			tilePx + 2,
			tilePx + 2,
			Color{ 255, 210, 50, 255 });
	}

	Vector2D playerPos = ctx.player()->position;
	DrawRectangle(
		originX + playerPos.x * tilePx - 1,
		originY + playerPos.y * tilePx - 1,
		tilePx + 2,
		tilePx + 2,
		Color{ 255, 255, 0, 255 });
}
