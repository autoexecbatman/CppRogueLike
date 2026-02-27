// file: AnimationSystem.h
#pragma once

#include <vector>

class Renderer;

struct AnimEntry
{
	int world_x;
	int world_y;
	int tile_id;
	unsigned char r, g, b;
	float spawn_time;
	float duration;
};

class AnimationSystem
{
public:
	void spawn_melee_hit(int world_x, int world_y);
	void spawn_death(int world_x, int world_y);

	void spawn_effect(
		int world_x,
		int world_y,
		int tile_id,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		float duration);

	void update_and_render(const Renderer& renderer);

private:
	std::vector<AnimEntry> entries;
};
