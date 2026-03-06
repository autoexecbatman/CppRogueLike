// file: AnimationSystem.h
#pragma once

#include <vector>

#include "../Renderer/Renderer.h"

class TileConfig;

struct AnimEntry
{
	int world_x;
	int world_y;
	TileRef tile;
	unsigned char r, g, b;
	float spawn_time;
	float duration;
};

class AnimationSystem
{
public:
	void init(const TileConfig& tc);
	void spawn_melee_hit(int world_x, int world_y);
	void spawn_death(int world_x, int world_y);

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
};
