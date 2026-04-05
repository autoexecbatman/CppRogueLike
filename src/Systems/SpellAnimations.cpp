// file: SpellAnimations.cpp
#include <vector>

#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Vector2D.h"
#include "AnimationSystem.h"
#include "SpellAnimations.h"

namespace SpellAnimations
{

void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
{
	if (!ctx.animSystem || !ctx.renderer)
		return;

	auto path = Map::bresenham_line(from, to);
	ctx.animSystem->spawn_lightning_path(path, 180, 220, 255);
	ctx.animSystem->spawn_spark_burst(to.x, to.y, 8, 140, 180, 255);
	ctx.renderer->add_trauma(0.25f);
}

void animate_explosion(Vector2D center, int radius, GameContext& ctx)
{
	if (!ctx.animSystem || !ctx.renderer)
		return;

	// Spawn sparks radiating outward from every cell in the blast radius
	for (int dy = -radius; dy <= radius; ++dy)
	{
		for (int dx = -radius; dx <= radius; ++dx)
		{
			if (dx * dx + dy * dy <= radius * radius)
			{
				ctx.animSystem->spawn_spark_burst(
					center.x + dx,
					center.y + dy,
					3,
					255,
					140,
					20);
			}
		}
	}

	ctx.animSystem->spawn_spark_burst(center.x, center.y, 16, 255, 80, 0);
	ctx.renderer->add_trauma(0.5f);
}

void animate_creature_hit(Vector2D position, GameContext& ctx)
{
	if (!ctx.animSystem)
		return;

	ctx.animSystem->spawn_blood_burst(position.x, position.y, 4);
}

} // namespace SpellAnimations
