// file: AnimationSystem.cpp
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <vector>

#include <raylib.h>

#include "../Renderer/Renderer.h"
#include "../Utils/Vector2D.h"
#include "AnimationSystem.h"
#include "TileConfig.h"

float AnimationSystem::random_range(float lo, float hi)
{
	std::uniform_real_distribution<float> dist(lo, hi);
	return dist(m_rng);
}

void AnimationSystem::init(const TileConfig& tileConfig, int tile_size)
{
	m_tile_size = tile_size;
	m_blood_tile = tileConfig.get("TILE_EFFECT_BLOOD");
	// Spark: fall back to blood tile until TILE_EFFECT_SPARK is added to tile_config.json.
	m_spark_tile = m_blood_tile;
	m_missile_tile = tileConfig.get("TILE_EFFECT_MISSILE");

	auto seed = static_cast<unsigned>(
		std::chrono::steady_clock::now().time_since_epoch().count());
	m_rng.seed(seed);
}

void AnimationSystem::spawn_blood_burst(int world_x, int world_y, int count)
{
	float cx = static_cast<float>(world_x * m_tile_size + m_tile_size / 2);
	float cy = static_cast<float>(world_y * m_tile_size + m_tile_size / 2);
	float now = static_cast<float>(GetTime());

	for (int i = 0; i < count; ++i)
	{
		float speed = random_range(40.0f, 130.0f);
		float angle = random_range(0.0f, 6.2832f);
		float dur = random_range(0.25f, 0.55f);
		unsigned char bright = static_cast<unsigned char>(random_range(180.0f, 255.0f));

		entries.push_back(AnimEntry{
			.px_x = cx,
			.px_y = cy,
			.vel_x = std::cos(angle) * speed,
			.vel_y = std::sin(angle) * speed,
			.radius = random_range(2.0f, 5.0f),
			.tile = {},
			.r = bright,
			.g = 20,
			.b = 20,
			.spawn_time = now,
			.duration = dur,
			.shape = ParticleShape::CIRCLE,
			.additive = false });
	}
}

void AnimationSystem::spawn_melee_hit(int world_x, int world_y)
{
	spawn_blood_burst(world_x, world_y, 5);
}

void AnimationSystem::spawn_death(int world_x, int world_y)
{
	spawn_blood_burst(world_x, world_y, 12);
}

void AnimationSystem::spawn_spark_burst(
	int world_x,
	int world_y,
	int count,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	float cx = static_cast<float>(world_x * m_tile_size + m_tile_size / 2);
	float cy = static_cast<float>(world_y * m_tile_size + m_tile_size / 2);
	float now = static_cast<float>(GetTime());

	for (int i = 0; i < count; ++i)
	{
		float speed = random_range(60.0f, 200.0f);
		float angle = random_range(0.0f, 6.2832f);
		float dur = random_range(0.3f, 0.7f);

		entries.push_back(AnimEntry{
			.px_x = cx,
			.px_y = cy,
			.vel_x = std::cos(angle) * speed,
			.vel_y = std::sin(angle) * speed,
			.radius = random_range(2.0f, 5.0f),
			.tile = {},
			.r = r,
			.g = g,
			.b = b,
			.spawn_time = now,
			.duration = dur,
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}
}

void AnimationSystem::spawn_lightning_path(
	const std::vector<Vector2D>& path,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	if (path.empty())
		return;

	float now = static_cast<float>(GetTime());

	for (const auto& pos : path)
	{
		float cx = static_cast<float>(pos.x * m_tile_size);
		float cy = static_cast<float>(pos.y * m_tile_size);

		entries.push_back(AnimEntry{
			.px_x = cx,
			.px_y = cy,
			.vel_x = 0.0f,
			.vel_y = 0.0f,
			.radius = static_cast<float>(m_tile_size) * 0.5f,
			.tile = m_spark_tile,
			.r = r,
			.g = g,
			.b = b,
			.spawn_time = now,
			.duration = 0.18f,
			.shape = ParticleShape::TILE,
			.additive = true });
	}
}

void AnimationSystem::spawn_effect(
	int world_x,
	int world_y,
	TileRef tile,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	float duration)
{
	float cx = static_cast<float>(world_x * m_tile_size);
	float cy = static_cast<float>(world_y * m_tile_size);

	entries.push_back(AnimEntry{
		.px_x = cx,
		.px_y = cy,
		.vel_x = 0.0f,
		.vel_y = 0.0f,
		.radius = static_cast<float>(m_tile_size),
		.tile = tile,
		.r = r,
		.g = g,
		.b = b,
		.spawn_time = static_cast<float>(GetTime()),
		.duration = duration,
		.shape = ParticleShape::TILE,
		.additive = false });
}

void AnimationSystem::spawn_projectile(
	Vector2D from,
	Vector2D to,
	TileRef tile,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	float speed,
	float wobbleStrength,
	std::function<void()> onArrive)
{
	float fromPxX = static_cast<float>(from.x * m_tile_size + m_tile_size / 2);
	float fromPxY = static_cast<float>(from.y * m_tile_size + m_tile_size / 2);
	float toPxX = static_cast<float>(to.x * m_tile_size + m_tile_size / 2);
	float toPxY = static_cast<float>(to.y * m_tile_size + m_tile_size / 2);

	float dx = toPxX - fromPxX;
	float dy = toPxY - fromPxY;
	float dist = std::sqrt(dx * dx + dy * dy);
	float now = static_cast<float>(GetTime());

	projectiles.push_back(ProjectileEntry{
		.pxX = fromPxX,
		.pxY = fromPxY,
		.targetPxX = toPxX,
		.targetPxY = toPxY,
		.speed = speed,
		.wobbleStrength = wobbleStrength,
		.initialDistance = std::max(dist, 1.0f),
		.lastTrailTime = now,
		.tile = tile,
		.r = r,
		.g = g,
		.b = b,
		.spawnTime = now,
		.maxDuration = (dist / speed) * 3.0f + 1.0f,
		.onArrive = std::move(onArrive) });
}

void AnimationSystem::update_and_render(const Renderer& renderer)
{
	float now = static_cast<float>(GetTime());
	float dt = GetFrameTime();
	int cam_x = renderer.get_camera_x();
	int cam_y = renderer.get_camera_y();

	auto is_expired = [&](const AnimEntry& e)
	{
		return (now - e.spawn_time) >= e.duration;
	};

	for (auto& e : entries)
	{
		if (is_expired(e))
			continue;

		// Integrate velocity
		e.px_x += e.vel_x * dt;
		e.px_y += e.vel_y * dt;
		// Dampen velocity (drag)
		e.vel_x *= (1.0f - dt * 4.0f);
		e.vel_y *= (1.0f - dt * 4.0f);

		float t = (now - e.spawn_time) / e.duration;

		// Flash on, then quadratic falloff
		float alpha_f = (t < 0.15f)
			? 1.0f
			: 1.0f - ((t - 0.15f) / 0.85f) * ((t - 0.15f) / 0.85f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = static_cast<int>(e.px_x) - cam_x;
		int screen_y = static_cast<int>(e.px_y) - cam_y;

		Color tint{ e.r, e.g, e.b, alpha };

		if (e.additive)
			BeginBlendMode(BLEND_ADDITIVE);

		switch (e.shape)
		{
		case ParticleShape::CIRCLE:
			DrawCircle(screen_x, screen_y, e.radius, tint);
			break;

		case ParticleShape::TILE:
		{
			int sz = static_cast<int>(e.radius);
			renderer.draw_tile_screen_color_sized(
				Vector2D{ screen_x - sz / 2, screen_y - sz / 2 },
				sz,
				e.tile,
				tint);
			break;
		}
		}

		if (e.additive)
			EndBlendMode();
	}

	std::erase_if(entries, is_expired);

	// Update and render seeking projectiles
	const float arrivalThreshold = static_cast<float>(m_tile_size) * 0.5f;

	for (auto& p : projectiles)
	{
		float dx = p.targetPxX - p.pxX;
		float dy = p.targetPxY - p.pxY;
		float dist = std::sqrt(dx * dx + dy * dy);

		if (dist < arrivalThreshold)
		{
			if (p.onArrive)
			{
				p.onArrive();
				p.onArrive = nullptr;
			}
			continue;
		}

		// Normalized direction toward target
		float nx = dx / dist;
		float ny = dy / dist;

		// Perpendicular axis for wobble
		float perpX = -ny;
		float perpY = nx;

		// Wobble shrinks as missile closes in
		float wobbleAmount = p.wobbleStrength * (dist / p.initialDistance);
		float noise = random_range(-1.0f, 1.0f);

		float dirX = nx + perpX * noise * wobbleAmount;
		float dirY = ny + perpY * noise * wobbleAmount;

		float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
		if (dirLen > 0.001f)
		{
			dirX /= dirLen;
			dirY /= dirLen;
		}

		p.pxX += dirX * p.speed * dt;
		p.pxY += dirY * p.speed * dt;

		// Emit trail particle
		if (now - p.lastTrailTime > 0.03f)
		{
			p.lastTrailTime = now;
			entries.push_back(AnimEntry{
				.px_x = p.pxX,
				.px_y = p.pxY,
				.vel_x = 0.0f,
				.vel_y = 0.0f,
				.radius = static_cast<float>(m_tile_size) * 0.3f,
				.tile = p.tile,
				.r = p.r,
				.g = p.g,
				.b = p.b,
				.spawn_time = now,
				.duration = 0.12f,
				.shape = ParticleShape::TILE,
				.additive = true });
		}

		// Render projectile head
		int screenX = static_cast<int>(p.pxX) - cam_x;
		int screenY = static_cast<int>(p.pxY) - cam_y;
		int sz = m_tile_size;

		BeginBlendMode(BLEND_ADDITIVE);
		renderer.draw_tile_screen_color_sized(
			Vector2D{ screenX - sz / 2, screenY - sz / 2 },
			sz,
			p.tile,
			Color{ p.r, p.g, p.b, 255 });
		EndBlendMode();
	}

	auto is_projectile_done = [&](const ProjectileEntry& p)
	{
		return !p.onArrive || (now - p.spawnTime) >= p.maxDuration;
	};
	std::erase_if(projectiles, is_projectile_done);
}
