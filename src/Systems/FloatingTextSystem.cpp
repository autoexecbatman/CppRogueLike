// file: FloatingTextSystem.cpp
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "../Renderer/Renderer.h"
#include "FloatingTextSystem.h"

void FloatingTextSystem::spawn_damage(
	int world_x,
	int world_y,
	int value,
	unsigned char r,
	unsigned char g,
	unsigned char b)
{
	entries.push_back(FloatingEntry{
		.world_x = world_x,
		.world_y = world_y,
		.text = std::format("{}", value),
		.r = r,
		.g = g,
		.b = b,
		.spawn_time = static_cast<float>(GetTime()),
		.lifetime = 1.2f });
}

void FloatingTextSystem::spawn_text(
	int world_x,
	int world_y,
	std::string text,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	float lifetime)
{
	entries.push_back(FloatingEntry{
		.world_x = world_x,
		.world_y = world_y,
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
	int ts = renderer.get_tile_size();
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
		float offset_px = t * static_cast<float>(ts) * 0.75f;

		// Full opacity for first 60%, then fade to zero
		float alpha_f = (t < 0.6f)
			? 1.0f
			: 1.0f - ((t - 0.6f) / 0.4f);
		unsigned char alpha = static_cast<unsigned char>(alpha_f * 255.0f);

		int screen_x = e.world_x * ts - cam_x;
		int screen_y = e.world_y * ts - cam_y - static_cast<int>(offset_px);

		Color col{ e.r, e.g, e.b, alpha };
		renderer.draw_text_color(screen_x, screen_y, e.text, col);
	}

	std::erase_if(entries, is_expired);
}
