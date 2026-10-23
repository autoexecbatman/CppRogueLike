// file: SpellAnimations.cpp
#include <cassert>

#include "AnimationBarrierMenu.h"
#include "AnimationSystem.h"
#include "GameContext.h"
#include "Map.h"
#include "Renderer.h"
#include "SpellAnimations.h"
#include "Vector2D.h"

namespace SpellAnimations
{

void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
{
	assert(
		ctx.animSystem &&
		"SpellAnimations::animate_lightning: called without an animSystem");
	if (!ctx.renderer)
	{
		return;
	}

	auto path = Map::bresenham_line(from, to);
	ctx.animSystem->spawn_lightning_path(path, 180, 220, 255);
	ctx.animSystem->spawn_spark_burst(to, 8, 140, 180, 255);
	ctx.renderer->add_trauma(0.25f);
}

void animate_explosion(Vector2D center, int radius, GameContext& ctx)
{
	assert(
		ctx.animSystem &&
		"SpellAnimations::animate_explosion: called without an animSystem");
	if (!ctx.renderer)
	{
		return;
	}

	ctx.animSystem->spawn_fireball_explosion(center, radius);
	ctx.renderer->add_trauma(0.5f);

	// Inside a menu the AnimationSystem is not ticked, so a barrier menu renders the world until the
	// particles expire, then pops itself.
	if (ctx.menus && !ctx.menus->empty())
	{
		ctx.menus->push_back(std::make_unique<AnimationBarrierMenu>(ctx));
	}
}

void animate_creature_hit(Vector2D position, GameContext& ctx)
{
	assert(
		ctx.animSystem &&
		"SpellAnimations::animate_creature_hit: called without an animSystem");

	ctx.animSystem->spawn_blood_burst(position, 4);
}

void animate_magic_missile(Vector2D from, Vector2D to, GameContext& ctx)
{
	assert(
		ctx.animSystem &&
		"SpellAnimations::animate_magic_missile: called without an animSystem");

	TileRef missileTile = ctx.animSystem->get_missile_tile();

	auto onArrive = [to, &ctx]()
	{
		assert(
			ctx.animSystem &&
			"SpellAnimations::animate_magic_missile: a missile arrived without an animSystem");
		ctx.animSystem->spawn_blood_burst(to, 4);
	};

	ctx.animSystem->spawn_projectile(
		from,
		to,
		missileTile,
		200,
		220,
		255,
		400.0f,
		2.5f,
		std::move(onArrive));
}

} // namespace SpellAnimations
