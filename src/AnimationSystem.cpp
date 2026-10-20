// file: AnimationSystem.cpp
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <vector>

#include <raylib.h>

#include "AnimationSystem.h"
#include "Renderer.h"
#include "TileConfig.h"
#include "Vector2D.h"

float AnimationSystem::random_range(float low, float high)
{
	std::uniform_real_distribution<float> dist(low, high);
	return dist(generator);
}

void AnimationSystem::init(const TileConfig& tileConfig, int tile_size)
{
	tileSize = tile_size;
	bloodTile = tileConfig.get("TILE_EFFECT_BLOOD");
	// Spark: fall back to blood tile until TILE_EFFECT_SPARK is added to tile_config.json.
	sparkTile = bloodTile;
	missileTile = tileConfig.get("TILE_EFFECT_MISSILE");

	auto seed = static_cast<unsigned>(
		std::chrono::steady_clock::now().time_since_epoch().count());
	generator.seed(seed);
}

void AnimationSystem::spawn_blood_burst(Vector2D worldPosition, int count)
{
	float centerPixelX = static_cast<float>(worldPosition.x * tileSize + tileSize / 2);
	float centerPixelY = static_cast<float>(worldPosition.y * tileSize + tileSize / 2);
	float now = static_cast<float>(GetTime());

	for (int particleIndex = 0; particleIndex < count; ++particleIndex)
	{
		float speed = random_range(40.0f, 130.0f);
		float angle = random_range(0.0f, 6.2832f);
		float dur = random_range(0.25f, 0.55f);
		unsigned char bright = static_cast<unsigned char>(random_range(180.0f, 255.0f));

		entries.push_back(AnimEntry{
			.pixelX = centerPixelX,
			.pixelY = centerPixelY,
			.velocityX = std::cos(angle) * speed,
			.velocityY = std::sin(angle) * speed,
			.radius = random_range(2.0f, 5.0f),
			.tile = {},
			.red = bright,
			.green = 20,
			.blue = 20,
			.spawnTime = now,
			.duration = dur,
			.shape = ParticleShape::CIRCLE,
			.additive = false });
	}
}

void AnimationSystem::spawn_melee_hit(Vector2D worldPosition)
{
	spawn_blood_burst(worldPosition, 5);
}

void AnimationSystem::spawn_death(Vector2D worldPosition)
{
	spawn_blood_burst(worldPosition, 12);
}

void AnimationSystem::spawn_spark_burst(
	Vector2D worldPosition,
	int count,
	unsigned char red,
	unsigned char green,
	unsigned char blue)
{
	float centerPixelX = static_cast<float>(worldPosition.x * tileSize + tileSize / 2);
	float centerPixelY = static_cast<float>(worldPosition.y * tileSize + tileSize / 2);
	float now = static_cast<float>(GetTime());

	for (int particleIndex = 0; particleIndex < count; ++particleIndex)
	{
		float speed = random_range(60.0f, 200.0f);
		float angle = random_range(0.0f, 6.2832f);
		float dur = random_range(0.3f, 0.7f);

		entries.push_back(AnimEntry{
			.pixelX = centerPixelX,
			.pixelY = centerPixelY,
			.velocityX = std::cos(angle) * speed,
			.velocityY = std::sin(angle) * speed,
			.radius = random_range(2.0f, 5.0f),
			.tile = {},
			.red = red,
			.green = green,
			.blue = blue,
			.spawnTime = now,
			.duration = dur,
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}
}

void AnimationSystem::spawn_lightning_path(
	const std::vector<Vector2D>& path,
	unsigned char red,
	unsigned char green,
	unsigned char blue)
{
	if (path.empty())
	{
		return;
	}

	float now = static_cast<float>(GetTime());

	for (const auto& pos : path)
	{
		float pixelX = static_cast<float>(pos.x * tileSize);
		float pixelY = static_cast<float>(pos.y * tileSize);

		entries.push_back(AnimEntry{
			.pixelX = pixelX,
			.pixelY = pixelY,
			.velocityX = 0.0f,
			.velocityY = 0.0f,
			.radius = static_cast<float>(tileSize) * 0.5f,
			.tile = sparkTile,
			.red = red,
			.green = green,
			.blue = blue,
			.spawnTime = now,
			.duration = 0.18f,
			.shape = ParticleShape::TILE,
			.additive = true });
	}
}

void AnimationSystem::spawn_effect(
	Vector2D worldPosition,
	TileRef tile,
	unsigned char red,
	unsigned char green,
	unsigned char blue,
	float duration)
{
	float pixelX = static_cast<float>(worldPosition.x * tileSize);
	float pixelY = static_cast<float>(worldPosition.y * tileSize);

	entries.push_back(AnimEntry{
		.pixelX = pixelX,
		.pixelY = pixelY,
		.velocityX = 0.0f,
		.velocityY = 0.0f,
		.radius = static_cast<float>(tileSize),
		.tile = tile,
		.red = red,
		.green = green,
		.blue = blue,
		.spawnTime = static_cast<float>(GetTime()),
		.duration = duration,
		.shape = ParticleShape::TILE,
		.additive = false });
}

void AnimationSystem::spawn_projectile(
	Vector2D from,
	Vector2D to,
	TileRef tile,
	unsigned char red,
	unsigned char green,
	unsigned char blue,
	float speed,
	float wobbleStrength,
	std::function<void()> onArrive)
{
	float fromPxX = static_cast<float>(from.x * tileSize + tileSize / 2);
	float fromPxY = static_cast<float>(from.y * tileSize + tileSize / 2);
	float toPxX = static_cast<float>(to.x * tileSize + tileSize / 2);
	float toPxY = static_cast<float>(to.y * tileSize + tileSize / 2);

	float deltaX = toPxX - fromPxX;
	float deltaY = toPxY - fromPxY;
	float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);
	float now = static_cast<float>(GetTime());

	projectiles.push_back(ProjectileEntry{
		.pixelX = fromPxX,
		.pixelY = fromPxY,
		.targetPixelX = toPxX,
		.targetPixelY = toPxY,
		.speed = speed,
		.wobbleStrength = wobbleStrength,
		.initialDistance = std::max(dist, 1.0f),
		.lastTrailTime = now,
		.tile = tile,
		.red = red,
		.green = green,
		.blue = blue,
		.spawnTime = now,
		.maxDuration = (dist / speed) * 3.0f + 1.0f,
		.onArrive = std::move(onArrive) });
}

void AnimationSystem::update_and_render(const Renderer& renderer)
{
	float now = static_cast<float>(GetTime());
	float frameSeconds = GetFrameTime();
	int cam_x = renderer.get_camera_x();
	int cam_y = renderer.get_camera_y();

	auto is_expired = [&](const AnimEntry& particle)
	{
		return (now - particle.spawnTime) >= particle.duration;
	};

	for (auto& particle : entries)
	{
		if (is_expired(particle))
		{
			continue;
		}

		// Integrate velocity
		particle.pixelX += particle.velocityX * frameSeconds;
		particle.pixelY += particle.velocityY * frameSeconds;
		// Buoyancy applied before drag so upward force fights deceleration each frame
		particle.velocityY += particle.accelerationY * frameSeconds;
		// Turbulence: perpendicular nudge each frame breaks clean arcs into chaotic paths
		if (particle.turbulence > 0.0f)
		{
			particle.velocityX += -particle.velocityY * particle.turbulence * frameSeconds;
			particle.velocityY += particle.velocityX * particle.turbulence * frameSeconds;
		}
		// Dampen velocity (drag)
		particle.velocityX *= (1.0f - frameSeconds * 4.0f);
		particle.velocityY *= (1.0f - frameSeconds * 4.0f);

		float lifeFraction = (now - particle.spawnTime) / particle.duration;

		// Particle has a future spawnTime - not yet visible
		if (lifeFraction < 0.0f)
		{
			continue;
		}

		// Flash on, then quadratic falloff
		float alpha_f = (lifeFraction < 0.15f)
			? 1.0f
			: 1.0f - ((lifeFraction - 0.15f) / 0.85f) * ((lifeFraction - 0.15f) / 0.85f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = static_cast<int>(particle.pixelX) - cam_x;
		int screen_y = static_cast<int>(particle.pixelY) - cam_y;

		// Color-over-lifetime: interpolate from spawn color to end color
		auto lerp_channel = [](unsigned char startValue, unsigned char endValue, float ratio) -> unsigned char
		{
			return static_cast<unsigned char>(
				static_cast<float>(startValue) + (static_cast<float>(endValue) - static_cast<float>(startValue)) * ratio);
		};
		Color tint{
			lerp_channel(particle.red, particle.redEnd, lifeFraction),
			lerp_channel(particle.green, particle.greenEnd, lifeFraction),
			lerp_channel(particle.blue, particle.blueEnd, lifeFraction),
			alpha
		};

		if (particle.additive)
		{
			BeginBlendMode(BLEND_ADDITIVE);
		}

		switch (particle.shape)
		{
		case ParticleShape::CIRCLE:
		{
			DrawCircle(screen_x, screen_y, particle.radius, tint);
			break;
		}

		case ParticleShape::TILE:
		{
			int sizePixels = static_cast<int>(particle.radius);
			renderer.draw_tile_screen_color_sized(
				Vector2D{ screen_x - sizePixels / 2, screen_y - sizePixels / 2 },
				sizePixels,
				particle.tile,
				tint);
			break;
		}

		case ParticleShape::PIXEL:
		{
			DrawPixel(screen_x, screen_y, tint);
			break;
		}
		}

		if (particle.additive)
		{
			EndBlendMode();
		}
	}

	std::erase_if(entries, is_expired);

	// Update and render seeking projectiles
	const float arrivalThreshold = static_cast<float>(tileSize) * 0.5f;

	for (auto& projectile : projectiles)
	{
		float deltaX = projectile.targetPixelX - projectile.pixelX;
		float deltaY = projectile.targetPixelY - projectile.pixelY;
		float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);

		if (dist < arrivalThreshold)
		{
			if (projectile.onArrive)
			{
				projectile.onArrive();
				projectile.onArrive = nullptr;
			}
			continue;
		}

		// Normalized direction toward target
		float unitX = deltaX / dist;
		float unitY = deltaY / dist;

		// Perpendicular axis for wobble
		float perpX = -unitY;
		float perpY = unitX;

		// Wobble shrinks as missile closes in
		float wobbleAmount = projectile.wobbleStrength * (dist / projectile.initialDistance);
		float noise = random_range(-1.0f, 1.0f);

		float dirX = unitX + perpX * noise * wobbleAmount;
		float dirY = unitY + perpY * noise * wobbleAmount;

		float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
		if (dirLen > 0.001f)
		{
			dirX /= dirLen;
			dirY /= dirLen;
		}

		projectile.pixelX += dirX * projectile.speed * frameSeconds;
		projectile.pixelY += dirY * projectile.speed * frameSeconds;

		// Emit trail particle
		if (now - projectile.lastTrailTime > 0.03f)
		{
			projectile.lastTrailTime = now;
			entries.push_back(AnimEntry{
				.pixelX = projectile.pixelX,
				.pixelY = projectile.pixelY,
				.velocityX = 0.0f,
				.velocityY = 0.0f,
				.radius = static_cast<float>(tileSize) * 0.3f,
				.tile = projectile.tile,
				.red = projectile.red,
				.green = projectile.green,
				.blue = projectile.blue,
				.spawnTime = now,
				.duration = 0.12f,
				.shape = ParticleShape::TILE,
				.additive = true });
		}

		// Render projectile head
		int screenX = static_cast<int>(projectile.pixelX) - cam_x;
		int screenY = static_cast<int>(projectile.pixelY) - cam_y;
		int sizePixels = tileSize;

		BeginBlendMode(BLEND_ADDITIVE);
		renderer.draw_tile_screen_color_sized(
			Vector2D{ screenX - sizePixels / 2, screenY - sizePixels / 2 },
			sizePixels,
			projectile.tile,
			Color{ projectile.red, projectile.green, projectile.blue, 255 });
		EndBlendMode();
	}

	auto is_projectile_done = [&](const ProjectileEntry& projectile)
	{
		return !projectile.onArrive || (now - projectile.spawnTime) >= projectile.maxDuration;
	};
	std::erase_if(projectiles, is_projectile_done);
}

void AnimationSystem::spawn_fireball_explosion(Vector2D center, int radius)
{
	float now = static_cast<float>(GetTime());
	float tilePixels = static_cast<float>(tileSize);
	float centerPixelX = static_cast<float>(center.x * tileSize + tileSize / 2);
	float centerPixelY = static_cast<float>(center.y * tileSize + tileSize / 2);
	float blastRadius = static_cast<float>(radius);
	float blastPx = blastRadius * tilePixels;

	// Phase 0: ignition flash - slow white-hot circles stacking additively into a solid glow.
	// Color-over-lifetime: white-yellow -> orange as the ball expands and cools.
	int flashCount = static_cast<int>(blastRadius * blastRadius * 14.0f);
	for (int flashIndex = 0; flashIndex < flashCount; ++flashIndex)
	{
		// Uniform random position within a circle of radius blastPx
		float distance = blastPx * std::sqrt(random_range(0.0f, 1.0f));
		float bearing = random_range(0.0f, 6.2832f);
		entries.push_back(AnimEntry{
			.pixelX = centerPixelX + distance * std::cos(bearing),
			.pixelY = centerPixelY + distance * std::sin(bearing),
			.velocityX = random_range(-25.0f, 25.0f),
			.velocityY = random_range(-25.0f, 25.0f),
			.accelerationY = 0.0f,
			.turbulence = 0.0f,
			.radius = random_range(4.0f, 8.0f),
			.tile = {},
			.red = 255,
			.green = 230,
			.blue = 140,
			.redEnd = 255,
			.greenEnd = 80,
			.blueEnd = 0,
			.spawnTime = now,
			.duration = random_range(0.06f, 0.14f),
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}

	// Phase 1: fireball body - circles staggered by distance, dense at the core and sparse at the edge.
	// Color-over-lifetime: yellow-orange -> dark red (temperature drop over time).
	int bodyCount = static_cast<int>(blastRadius * blastRadius * 18.0f);
	for (int bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex)
	{
		float distance = blastPx * std::sqrt(random_range(0.0f, 1.0f));
		float bearing = random_range(0.0f, 6.2832f);
		float distRatio = distance / blastPx;

		// Stagger: outer circles appear slightly later - expanding ball feel
		float spawnAt = now + distRatio * 0.12f;

		// Color: inner = yellow-white, outer = orange-red
		unsigned char spawnG = static_cast<unsigned char>(180.0f * (1.0f - distRatio * 0.55f));
		unsigned char spawnB = static_cast<unsigned char>(40.0f * (1.0f - distRatio));
		unsigned char endG = static_cast<unsigned char>(15.0f * (1.0f - distRatio * 0.7f));

		// Outer circles drift outward slightly; inner circles barely move
		float driftSpeed = distRatio * 35.0f;
		entries.push_back(AnimEntry{
			.pixelX = centerPixelX + distance * std::cos(bearing),
			.pixelY = centerPixelY + distance * std::sin(bearing),
			.velocityX = std::cos(bearing) * driftSpeed + random_range(-10.0f, 10.0f),
			.velocityY = std::sin(bearing) * driftSpeed + random_range(-10.0f, 10.0f),
			.accelerationY = 0.0f,
			.turbulence = 0.0f,
			.radius = random_range(3.5f, 7.0f) * (1.0f - distRatio * 0.4f),
			.tile = {},
			.red = 255,
			.green = spawnG,
			.blue = spawnB,
			.redEnd = 200,
			.greenEnd = endG,
			.blueEnd = 0,
			.spawnTime = spawnAt,
			.duration = random_range(0.35f, 0.75f),
			.shape = ParticleShape::CIRCLE,
			.additive = true });
	}

	// Phase 2: radial sparks - fast pixels at the blast edge, the "pop" of the explosion.
	// These are debris and hot gas ejected outward. Short duration, high velocity.
	for (int sparkIndex = 0; sparkIndex < 60; ++sparkIndex)
	{
		float angle = random_range(0.0f, 6.2832f);
		float speed = random_range(180.0f, 420.0f);
		entries.push_back(AnimEntry{
			.pixelX = centerPixelX,
			.pixelY = centerPixelY,
			.velocityX = std::cos(angle) * speed,
			.velocityY = std::sin(angle) * speed,
			.accelerationY = 0.0f,
			.turbulence = 0.0f,
			.radius = 1.0f,
			.tile = {},
			.red = 255,
			.green = 180,
			.blue = 60,
			.redEnd = 180,
			.greenEnd = 20,
			.blueEnd = 0,
			.spawnTime = now,
			.duration = random_range(0.20f, 0.45f),
			.shape = ParticleShape::PIXEL,
			.additive = true });
	}

	// Phase 3: embers - small count of rising pixels, subtle trailing effect only.
	// These represent hot debris carried upward after the ball dissipates.
	for (int emberIndex = 0; emberIndex < 30; ++emberIndex)
	{
		float distance = blastPx * random_range(0.0f, 0.6f);
		float bearing = random_range(0.0f, 6.2832f);
		entries.push_back(AnimEntry{
			.pixelX = centerPixelX + distance * std::cos(bearing),
			.pixelY = centerPixelY + distance * std::sin(bearing),
			.velocityX = random_range(-15.0f, 15.0f),
			.velocityY = random_range(-80.0f, -30.0f),
			.accelerationY = random_range(-60.0f, -30.0f),
			.turbulence = 0.0f,
			.radius = 1.0f,
			.tile = {},
			.red = 220,
			.green = 80,
			.blue = 10,
			.redEnd = 80,
			.greenEnd = 10,
			.blueEnd = 0,
			.spawnTime = now + random_range(0.15f, 0.55f),
			.duration = random_range(0.7f, 1.4f),
			.shape = ParticleShape::PIXEL,
			.additive = true });
	}
}
