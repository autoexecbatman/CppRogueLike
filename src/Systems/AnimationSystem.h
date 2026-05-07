// file: AnimationSystem.h
#pragma once

#include <functional>
#include <random>
#include <vector>

#include "../Renderer/Renderer.h"

class TileConfig;
struct Vector2D;

enum class ParticleShape
{
	CIRCLE,  // Raylib filled circle — blood, sparks
	TILE,    // DawnLike sprite — named effects
	PIXEL,   // Single DrawPixel — fire, embers
};

struct AnimEntry
{
	float px_x;               // world-space pixel position
	float px_y;
	float vel_x{ 0.0f };     // world-space pixels per second
	float vel_y{ 0.0f };
	float accel_y{ 0.0f };      // upward buoyancy — negative = rises; zero = drag only
	float turbulence{ 0.0f };   // perpendicular swirl per frame; 0 = straight arcs
	float radius{ 4.0f };       // pixels, for CIRCLE shape
	TileRef tile;               // used only for TILE shape
	unsigned char r, g, b;
	unsigned char r_end{ 0 };   // color at end of lifetime — interpolated over duration
	unsigned char g_end{ 0 };   // default 0,0,0 = cool to black
	unsigned char b_end{ 0 };
	float spawn_time;
	float duration;
	ParticleShape shape{ ParticleShape::CIRCLE };
	bool additive{ false };
};

struct ProjectileEntry
{
	float pxX;
	float pxY;
	float targetPxX;
	float targetPxY;
	float speed;
	float wobbleStrength;
	float initialDistance;
	float lastTrailTime{ 0.0f };
	TileRef tile;
	unsigned char r{ 255 };
	unsigned char g{ 255 };
	unsigned char b{ 255 };
	float spawnTime;
	float maxDuration;
	std::function<void()> onArrive;
};

class AnimationSystem
{
public:
	void init(const TileConfig& tileConfig, int tile_size);

	// Particle bursts
	void spawn_melee_hit(int world_x, int world_y);
	void spawn_death(int world_x, int world_y);
	void spawn_blood_burst(int world_x, int world_y, int count);
	void spawn_spark_burst(
		int world_x,
		int world_y,
		int count,
		unsigned char r,
		unsigned char g,
		unsigned char b);

	// Multi-phase fire explosion: radial wavefront, color gradient, rising embers
	void spawn_fireball_explosion(Vector2D center, int radius);

	// Lightning: flash tiles along a bresenham path
	void spawn_lightning_path(
		const std::vector<Vector2D>& path,
		unsigned char r,
		unsigned char g,
		unsigned char b);

	[[nodiscard]] TileRef get_missile_tile() const noexcept { return m_missile_tile; }

	// Seeking projectile with chaotic wobble motion
	void spawn_projectile(
		Vector2D from,
		Vector2D to,
		TileRef tile,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		float speed,
		float wobbleStrength,
		std::function<void()> onArrive);

	// Generic single effect
	void spawn_effect(
		int world_x,
		int world_y,
		TileRef tile,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		float duration);

	void update_and_render(const Renderer& renderer);

	[[nodiscard]] bool has_active_entries() const noexcept
	{
		return !entries.empty() || !projectiles.empty();
	}

private:
	std::vector<AnimEntry> entries;
	std::vector<ProjectileEntry> projectiles;
	TileRef m_blood_tile{};
	TileRef m_spark_tile{};
	TileRef m_missile_tile{};
	int m_tile_size{ 32 };
	std::mt19937 m_rng;

	float random_range(float lo, float hi);
};
