// file: FloatingTextSystem.cpp
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "FloatingTextSystem.h"
#include "Renderer.h"

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
		.red = 255,
		.green = static_cast<unsigned char>(hurtPlayer ? 80 : 220),
		.blue = static_cast<unsigned char>(hurtPlayer ? 80 : 50),
		.spawnTime = static_cast<float>(GetTime()),
		.lifetime = 1.2f,
	});
}

void FloatingTextSystem::spawn_text(
	Vector2D worldPosition,
	std::string text,
	unsigned char red,
	unsigned char green,
	unsigned char blue,
	float lifetime)
{
	entries.push_back(FloatingEntry{
		.worldPosition = worldPosition,
		.text = std::move(text),
		.red = red,
		.green = green,
		.blue = blue,
		.spawnTime = static_cast<float>(GetTime()),
		.lifetime = lifetime,
	});
}

void FloatingTextSystem::update_and_render(const Renderer& renderer)
{
	float now = static_cast<float>(GetTime());
	int tileSize = renderer.get_tile_size();
	int cam_x = renderer.get_camera_x();
	int cam_y = renderer.get_camera_y();

	auto is_expired = [&](const FloatingEntry& entry)
	{
		return (now - entry.spawnTime) >= entry.lifetime;
	};

	for (const auto& entry : entries)
	{
		if (is_expired(entry))
		{
			continue;
		}

		float lifeFraction = (now - entry.spawnTime) / entry.lifetime;

		// Float upward by 0.75 tiles over the lifetime
		float offset_px = lifeFraction * static_cast<float>(tileSize) * 0.75f;

		// Full opacity for first 60%, then fade to zero
		float alpha_f = (lifeFraction < 0.6f)
			? 1.0f
			: 1.0f - ((lifeFraction - 0.6f) / 0.4f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = entry.worldPosition.x * tileSize - cam_x;
		int screen_y = entry.worldPosition.y * tileSize - cam_y - static_cast<int>(offset_px);

		Color col{ entry.red, entry.green, entry.blue, alpha };
		renderer.draw_text_color(Vector2D{ screen_x, screen_y }, entry.text, col);
	}

	std::erase_if(entries, is_expired);
}
