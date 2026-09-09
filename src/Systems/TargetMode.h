#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

enum class TargetMode : int
{
	AUTO_NEAREST,
	PICK_TILE_SINGLE,
	PICK_TILE_AOE,
	FOV_BUFF
};
enum class ScrollAnimation : int
{
	NONE,
	LIGHTNING,
	EXPLOSION
};

inline std::string_view encode_scroll_animation(ScrollAnimation a)
{
	switch (a)
	{

	case ScrollAnimation::NONE:
	{
		return "none";
	}

	case ScrollAnimation::LIGHTNING:
	{
		return "lightning";
	}

	case ScrollAnimation::EXPLOSION:
	{
		return "explosion";
	}

	}

	return "none";
}

inline ScrollAnimation parse_scroll_animation(std::string_view s)
{
	if (s == "none")
	{
		return ScrollAnimation::NONE;
	}
	if (s == "lightning")
	{
		return ScrollAnimation::LIGHTNING;
	}
	if (s == "explosion")
	{
		return ScrollAnimation::EXPLOSION;
	}

	throw std::runtime_error(std::format("unknown scroll_animation '{}'", s));
}

inline std::string_view encode_target_mode(TargetMode m)
{
	switch (m)
	{

	case TargetMode::AUTO_NEAREST:
	{
		return "auto_nearest";
	}

	case TargetMode::PICK_TILE_SINGLE:
	{
		return "pick_tile_single";
	}

	case TargetMode::PICK_TILE_AOE:
	{
		return "pick_tile_aoe";
	}

	case TargetMode::FOV_BUFF:
	{
		return "fov_buff";
	}

	}

	return "auto_nearest";
}

inline TargetMode parse_target_mode(std::string_view s)
{
	if (s == "auto_nearest")
	{
		return TargetMode::AUTO_NEAREST;
	}
	if (s == "pick_tile_single")
	{
		return TargetMode::PICK_TILE_SINGLE;
	}
	if (s == "pick_tile_aoe")
	{
		return TargetMode::PICK_TILE_AOE;
	}
	if (s == "fov_buff")
	{
		return TargetMode::FOV_BUFF;
	}

	throw std::runtime_error(std::format("unknown target_mode '{}'", s));
}
