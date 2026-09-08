// file: FloatingTextSystem.cpp
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "../Renderer/Renderer.h"
#include "FloatingTextSystem.h"

// Shows a damage number rising off a tile.
//
// The colour is decided here rather than by the caller: red when the player is
// hurt, yellow otherwise, so a glance at the screen says whether the damage was
// yours. A caller states who was hit and nothing about how it looks.
//
// Example:
//   floatingText->spawn_damage(pos, 7, DamageSubject::PLAYER);   // red 7
//   floatingText->spawn_damage(pos, 7, DamageSubject::MONSTER);  // yellow 7
void FloatingTextSystem::spawn_damage(Vector2D worldPosition, int value, DamageSubject subject)
{
	// Damage to the player is the one a human needs to notice.
	const bool hurtPlayer = (subject == DamageSubject::PLAYER);

	entries.push_back(FloatingEntry{
		.worldPosition = worldPosition,
		.text = std::format("{}", value),
		.r = 255,
		.g = static_cast<unsigned char>(hurtPlayer ? 80 : 220),
		.b = static_cast<unsigned char>(hurtPlayer ? 80 : 50),
		.spawn_time = static_cast<float>(GetTime()),
		.lifetime = 1.2f });
}

void FloatingTextSystem::spawn_text(
	Vector2D worldPosition,
	std::string text,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	float lifetime)
{
	entries.push_back(FloatingEntry{
		.worldPosition = worldPosition,
		.text = std::move(text),
		.r = r,
		.g = g,
		.b = b,
		.spawn_time = static_cast<float>(GetTime()),
		.lifetime = lifetime });
}

void FloatingTextSystem::update_and_render(const Renderer& renderer)
{
	float now = static_cast<float>(GetTime());
	int tileSize = renderer.get_tile_size();
	int cam_x = renderer.get_camera_x();
	int cam_y = renderer.get_camera_y();

	auto is_expired = [&](const FloatingEntry& e)
	{
		return (now - e.spawn_time) >= e.lifetime;
	};

	for (const auto& e : entries)
	{
		if (is_expired(e))
		{
			continue;
		}

		float t = (now - e.spawn_time) / e.lifetime;

		// Float upward by 0.75 tiles over the lifetime
		float offset_px = t * static_cast<float>(tileSize) * 0.75f;

		// Full opacity for first 60%, then fade to zero
		float alpha_f = (t < 0.6f)
			? 1.0f
			: 1.0f - ((t - 0.6f) / 0.4f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = e.worldPosition.x * tileSize - cam_x;
		int screen_y = e.worldPosition.y * tileSize - cam_y - static_cast<int>(offset_px);

		Color col{ e.r, e.g, e.b, alpha };
		renderer.draw_text_color(Vector2D{ screen_x, screen_y }, e.text, col);
	}

	std::erase_if(entries, is_expired);
}
