// file: FloatingTextSystem.h
#pragma once

#include <string>
#include <vector>

#include "Vector2D.h"

class Renderer;

struct FloatingEntry
{
	Vector2D worldPosition{};
	std::string text{};
	unsigned char red{ 255 };
	unsigned char green{ 255 };
	unsigned char blue{ 255 };
	float spawnTime{ 0.0f };
	float lifetime{ 0.0f };
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
private:
	std::vector<FloatingEntry> entries;

public:
	// Shows a damage number rising off a tile, coloured by who was hit.
	void spawn_damage(Vector2D worldPosition, int value, DamageSubject subject);

	void spawn_text(
		Vector2D worldPosition,
		std::string text,
		unsigned char red,
		unsigned char green,
		unsigned char blue,
		float lifetime);

	void update_and_render(const Renderer& renderer);
};
