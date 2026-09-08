// file: FloatingTextSystem.h
#pragma once

#include <string>
#include <vector>

#include "../Utils/Vector2D.h"

class Renderer;

struct FloatingEntry
{
	Vector2D worldPosition{};
	std::string text;
	unsigned char r, g, b;
	float spawn_time;
	float lifetime;
};

// Who took the damage, which is all the palette needs to know. Callers state
// the fact; this system owns what colour it is drawn in.
enum class DamageSubject
{
	PLAYER,
	MONSTER
};

class FloatingTextSystem
{
public:
	// Shows a damage number rising off a tile, coloured by who was hit.
	void spawn_damage(Vector2D worldPosition, int value, DamageSubject subject);

	void spawn_text(
		Vector2D worldPosition,
		std::string text,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		float lifetime);

	void update_and_render(const Renderer& renderer);

private:
	std::vector<FloatingEntry> entries;
};
