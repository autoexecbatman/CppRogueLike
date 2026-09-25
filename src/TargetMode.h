#pragma once

#include <algorithm>
#include <array>

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

// Every TargetMode, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<TargetMode, 4> ALL_TARGET_MODE = {
	TargetMode::AUTO_NEAREST,
	TargetMode::PICK_TILE_SINGLE,
	TargetMode::PICK_TILE_AOE,
	TargetMode::FOV_BUFF,
};

// The next TargetMode in that order, wrapping at the end.
//
// Example:
//   next_target_mode(TargetMode::AUTO_NEAREST);   // -> TargetMode::PICK_TILE_SINGLE
[[nodiscard]] inline TargetMode next_target_mode(TargetMode value)
{
	const auto found = std::ranges::find(ALL_TARGET_MODE, value);
	if (found == ALL_TARGET_MODE.end() || found + 1 == ALL_TARGET_MODE.end())
	{
		return ALL_TARGET_MODE.front();
	}
	return *(found + 1);
}
enum class ScrollAnimation : int
{
	NONE,
	LIGHTNING,
	EXPLOSION
};

// Every ScrollAnimation, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<ScrollAnimation, 3> ALL_SCROLL_ANIMATION = {
	ScrollAnimation::NONE,
	ScrollAnimation::LIGHTNING,
	ScrollAnimation::EXPLOSION,
};

// The next ScrollAnimation in that order, wrapping at the end.
//
// Example:
//   next_scroll_animation(ScrollAnimation::NONE);   // -> ScrollAnimation::LIGHTNING
[[nodiscard]] inline ScrollAnimation next_scroll_animation(ScrollAnimation value)
{
	const auto found = std::ranges::find(ALL_SCROLL_ANIMATION, value);
	if (found == ALL_SCROLL_ANIMATION.end() || found + 1 == ALL_SCROLL_ANIMATION.end())
	{
		return ALL_SCROLL_ANIMATION.front();
	}
	return *(found + 1);
}

inline std::string_view encode_scroll_animation(ScrollAnimation scrollAnimation)
{
	switch (scrollAnimation)
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

inline ScrollAnimation parse_scroll_animation(std::string_view name)
{
	if (name == "none")
	{
		return ScrollAnimation::NONE;
	}
	if (name == "lightning")
	{
		return ScrollAnimation::LIGHTNING;
	}
	if (name == "explosion")
	{
		return ScrollAnimation::EXPLOSION;
	}

	throw std::runtime_error(std::format("unknown scroll_animation '{}'", name));
}

inline std::string_view encode_target_mode(TargetMode targetMode)
{
	switch (targetMode)
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

inline TargetMode parse_target_mode(std::string_view name)
{
	if (name == "auto_nearest")
	{
		return TargetMode::AUTO_NEAREST;
	}
	if (name == "pick_tile_single")
	{
		return TargetMode::PICK_TILE_SINGLE;
	}
	if (name == "pick_tile_aoe")
	{
		return TargetMode::PICK_TILE_AOE;
	}
	if (name == "fov_buff")
	{
		return TargetMode::FOV_BUFF;
	}

	throw std::runtime_error(std::format("unknown target_mode '{}'", name));
}
