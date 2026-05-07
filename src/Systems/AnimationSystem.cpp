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
		// Buoyancy applied before drag so upward force fights deceleration each frame
		e.vel_y += e.accel_y * dt;
		// Turbulence: perpendicular nudge each frame breaks clean arcs into chaotic paths
		if (e.turbulence > 0.0f)
		{
			e.vel_x += -e.vel_y * e.turbulence * dt;
			e.vel_y +=  e.vel_x * e.turbulence * dt;
		}
		// Dampen velocity (drag)
		e.vel_x *= (1.0f - dt * 4.0f);
		e.vel_y *= (1.0f - dt * 4.0f);

		float t = (now - e.spawn_time) / e.duration;

		// Particle has a future spawn_time — not yet visible
		if (t < 0.0f)
		{
			continue;
		}

		// Flash on, then quadratic falloff
		float alpha_f = (t < 0.15f)
			? 1.0f
			: 1.0f - ((t - 0.15f) / 0.85f) * ((t - 0.15f) / 0.85f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = static_cast<int>(e.px_x) - cam_x;
		int screen_y = static_cast<int>(e.px_y) - cam_y;

		// Color-over-lifetime: interpolate from spawn color to end color
		auto lerp_channel = [](unsigned char a, unsigned char b, float ratio) -> unsigned char
		{
			return static_cast<unsigned char>(
				static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * ratio);
		};
		Color tint{
			lerp_channel(e.r, e.r_end, t),
			lerp_channel(e.g, e.g_end, t),
			lerp_channel(e.b, e.b_end, t),
			alpha };

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

		case ParticleShape::PIXEL:
			DrawPixel(screen_x, screen_y, tint);
			break;
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

void AnimationSystem::spawn_fireball_explosion(Vector2D center, int radius)
{
	float now = static_cast<float>(GetTime());
	float ts = static_cast<float>(m_tile_size);
	float cx = static_cast<float>(center.x * m_tile_size + m_tile_size / 2);
	float cy = static_cast<float>(center.y * m_tile_size + m_tile_size / 2);
	float blastRadius = static_cast<float>(radius);
	float blastPx = blastRadius * ts;

	// Phase 0: ignition flash — dense white-hot circles fill the entire blast area.
	// LOW velocity: circles barely drift, they overlap and stack via additive blend
	// to create a solid white-yellow glow. This is the "ball of fire" appearance.
	// Color-over-lifetime: white-yellow → orange as the ball expands and cools.
	int flashCount = static_cast<int>(blastRadius * blastRadius * 14.0f);
	for (int i = 0; i < flashCount; ++i)
	{
		// Uniform random position within a circle of radius blastPx
		float r = blastPx * std::sqrt(random_range(0.0f, 1.0f));
		float a = random_range(0.0f, 6.2832f);
		entries.push_back(AnimEntry{
			.px_x = cx + r * std::cos(a),
			.px_y = cy + r * std::sin(a),
			.vel_x = random_range(-25.0f, 25.0f),
			.vel_y = random_range(-25.0f, 25.0f),
			.accel_y = 0.0f,
			.turbulence = 0.0f,
			.radius = random_range(4.0f, 8.0f),
			.tile = {},
			.r = 255, .g = 230, .b = 140,
			.r_end = 255, .g_end = 80, .b_end = 0,
			.spawn_time = now,
			.duration = random_range(0.06f, 0.14f),
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}

	// Phase 1: fireball body — circles spawned throughout blast area with low drift.
	// Staggered spawn (delay proportional to distance) gives the expanding ball effect.
	// Dense overlap at center = bright core. Sparse at edge = fire boundary.
	// Color-over-lifetime: yellow-orange → dark red (temperature drop over time).
	int bodyCount = static_cast<int>(blastRadius * blastRadius * 18.0f);
	for (int i = 0; i < bodyCount; ++i)
	{
		float r = blastPx * std::sqrt(random_range(0.0f, 1.0f));
		float a = random_range(0.0f, 6.2832f);
		float distRatio = r / blastPx;

		// Stagger: outer circles appear slightly later — expanding ball feel
		float spawnAt = now + distRatio * 0.12f;

		// Color: inner = yellow-white, outer = orange-red
		unsigned char spawnG = static_cast<unsigned char>(180.0f * (1.0f - distRatio * 0.55f));
		unsigned char spawnB = static_cast<unsigned char>(40.0f  * (1.0f - distRatio));
		unsigned char endG   = static_cast<unsigned char>(15.0f  * (1.0f - distRatio * 0.7f));

		// Outer circles drift outward slightly; inner circles barely move
		float driftSpeed = distRatio * 35.0f;
		entries.push_back(AnimEntry{
			.px_x = cx + r * std::cos(a),
			.px_y = cy + r * std::sin(a),
			.vel_x = std::cos(a) * driftSpeed + random_range(-10.0f, 10.0f),
			.vel_y = std::sin(a) * driftSpeed + random_range(-10.0f, 10.0f),
			.accel_y = 0.0f,
			.turbulence = 0.0f,
			.radius = random_range(3.5f, 7.0f) * (1.0f - distRatio * 0.4f),
			.tile = {},
			.r = 255, .g = spawnG, .b = spawnB,
			.r_end = 200, .g_end = endG, .b_end = 0,
			.spawn_time = spawnAt,
			.duration = random_range(0.35f, 0.75f),
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}

	// Phase 2: radial sparks — fast pixels at the blast edge, the "pop" of the explosion.
	// These are debris and hot gas ejected outward. Short duration, high velocity.
	for (int i = 0; i < 60; ++i)
	{
		float angle = random_range(0.0f, 6.2832f);
		float speed = random_range(180.0f, 420.0f);
		entries.push_back(AnimEntry{
			.px_x = cx,
			.px_y = cy,
			.vel_x = std::cos(angle) * speed,
			.vel_y = std::sin(angle) * speed,
			.accel_y = 0.0f,
			.turbulence = 0.0f,
			.radius = 1.0f,
			.tile = {},
			.r = 255, .g = 180, .b = 60,
			.r_end = 180, .g_end = 20, .b_end = 0,
			.spawn_time = now,
			.duration = random_range(0.20f, 0.45f),
			.shape = ParticleShape::PIXEL,
			.additive = true });
	}

	// Phase 3: embers — small count of rising pixels, subtle trailing effect only.
	// These represent hot debris carried upward after the ball dissipates.
	for (int i = 0; i < 30; ++i)
	{
		float r = blastPx * random_range(0.0f, 0.6f);
		float a = random_range(0.0f, 6.2832f);
		entries.push_back(AnimEntry{
			.px_x = cx + r * std::cos(a),
			.px_y = cy + r * std::sin(a),
			.vel_x = random_range(-15.0f, 15.0f),
			.vel_y = random_range(-80.0f, -30.0f),
			.accel_y = random_range(-60.0f, -30.0f),
			.turbulence = 0.0f,
			.radius = 1.0f,
			.tile = {},
			.r = 220, .g = 80, .b = 10,
			.r_end = 80, .g_end = 10, .b_end = 0,
			.spawn_time = now + random_range(0.15f, 0.55f),
			.duration = random_range(0.7f, 1.4f),
			.shape = ParticleShape::PIXEL,
			.additive = true });
	}
}
