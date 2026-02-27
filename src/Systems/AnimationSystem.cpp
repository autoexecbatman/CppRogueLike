// file: AnimationSystem.cpp
#include "AnimationSystem.h"

#include <algorithm>

#include "../Renderer/Renderer.h"
#include "../Renderer/TileId.h"

void AnimationSystem::spawn_melee_hit(int world_x, int world_y)
{
	entries.push_back(AnimEntry{
		.world_x = world_x,
		.world_y = world_y,
		.tile_id = TILE_EFFECT_BLOOD,
		.r = 255,
		.g = 60,
		.b = 60,
		.spawn_time = static_cast<float>(GetTime()),
		.duration = 0.25f });
}

void AnimationSystem::spawn_death(int world_x, int world_y)
{
	entries.push_back(AnimEntry{
		.world_x = world_x,
		.world_y = world_y,
		.tile_id = TILE_EFFECT_BLOOD,
		.r = 200,
		.g = 0,
		.b = 0,
		.spawn_time = static_cast<float>(GetTime()),
		.duration = 0.6f });
}

void AnimationSystem::spawn_effect(
	int world_x,
	int world_y,
	int tile_id,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	float duration)
{
	entries.push_back(AnimEntry{
		.world_x = world_x,
		.world_y = world_y,
		.tile_id = tile_id,
		.r = r,
		.g = g,
		.b = b,
		.spawn_time = static_cast<float>(GetTime()),
		.duration = duration });
}

void AnimationSystem::update_and_render(const Renderer& renderer)
{
	float now = static_cast<float>(GetTime());

	auto is_expired = [&](const AnimEntry& e)
	{
		return (now - e.spawn_time) >= e.duration;
	};

	for (const auto& e : entries)
	{
		if (is_expired(e))
			continue;

		float t = (now - e.spawn_time) / e.duration;

		// Flash on, then fade: full for first 20%, then quadratic falloff
		float alpha_f = (t < 0.2f)
			? 1.0f
			: 1.0f - ((t - 0.2f) / 0.8f) * ((t - 0.2f) / 0.8f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		Color tint{ e.r, e.g, e.b, alpha };
		renderer.draw_tile(e.world_x, e.world_y, e.tile_id, 0, tint);
	}

	std::erase_if(entries, is_expired);
}
