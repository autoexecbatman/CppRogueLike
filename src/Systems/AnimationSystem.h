// file: AnimationSystem.h
#pragma once

#include <random>
#include <vector>

#include "../Renderer/Renderer.h"

class TileConfig;
struct Vector2D;

enum class ParticleShape
{
	CIRCLE,  // Raylib filled circle — blood, sparks
	TILE,    // DawnLike sprite — named effects
};

struct AnimEntry
{
	float px_x;               // world-space pixel position
	float px_y;
	float vel_x{ 0.0f };     // world-space pixels per second
	float vel_y{ 0.0f };
	float radius{ 4.0f };     // pixels, for CIRCLE shape
	TileRef tile;             // used only for TILE shape
	unsigned char r, g, b;
	float spawn_time;
	float duration;
	ParticleShape shape{ ParticleShape::CIRCLE };
	bool additive{ false };
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

	// Lightning: flash tiles along a bresenham path
	void spawn_lightning_path(
		const std::vector<Vector2D>& path,
		unsigned char r,
		unsigned char g,
		unsigned char b);

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

private:
	std::vector<AnimEntry> entries;
	TileRef m_blood_tile{};
	TileRef m_spark_tile{};
	int m_tile_size{ 32 };
	std::mt19937 m_rng;

	float random_range(float lo, float hi);
};
