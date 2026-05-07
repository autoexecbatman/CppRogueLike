// file: SpellAnimations.cpp
#include <functional>
#include <vector>

#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Menu/AnimationBarrierMenu.h"
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

	ctx.animSystem->spawn_fireball_explosion(center, radius);
	ctx.renderer->add_trauma(0.5f);

	// When fired from inside menu context (e.g. scroll used from inventory),
	// the AnimationSystem is never ticked — only handle_gameloop calls it.
	// Push a barrier menu that renders the game world + animations until all
	// particles expire, then pops itself back to whatever was on the stack.
	if (ctx.menus && !ctx.menus->empty())
	{
		ctx.menus->push_back(std::make_unique<AnimationBarrierMenu>(ctx));
	}
}

void animate_creature_hit(Vector2D position, GameContext& ctx)
{
	if (!ctx.animSystem)
		return;

	ctx.animSystem->spawn_blood_burst(position.x, position.y, 4);
}

void animate_magic_missile(Vector2D from, Vector2D to, GameContext& ctx)
{
	if (!ctx.animSystem)
	{
		return;
	}

	TileRef missileTile = ctx.animSystem->get_missile_tile();

	auto onArrive = [to, &ctx]()
	{
		if (ctx.animSystem)
		{
			ctx.animSystem->spawn_blood_burst(to.x, to.y, 4);
		}
	};

	ctx.animSystem->spawn_projectile(
		from,
		to,
		missileTile,
		200, 220, 255,
		400.0f,
		2.5f,
		std::move(onArrive));
}

} // namespace SpellAnimations
