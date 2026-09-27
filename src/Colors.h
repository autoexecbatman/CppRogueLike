#pragma once

// file: Colors.h
//
// The colour pairs the game draws text in: a foreground and a background chosen
// together, named as one value. Everything that prints takes one of these rather
// than a number, so a pair that does not exist cannot be asked for.
//
// The identifiers are the pair's index into the renderer's table, which is why they
// start at one - slot zero is the table's default and belongs to no pair. Nothing
// outside the renderer needs to know that; the table's own size is derived here so
// it cannot fall behind the list.
//
// Usage:
//
//   ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, "You notice a trap!", true);
//
//   color_pair_name(ColorPairId::BROWN_BLACK);   // -> "brown_black", what data carries
//   parse_color_pair("brown_black");             // -> ColorPairId::BROWN_BLACK
//   parse_color_pair("puce");                    // throws, naming what it read
//
// The colours themselves live in Renderer.cpp, beside the table they fill, because
// they are raylib values and this header is included by fifty files that want none
// of raylib.

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <stdexcept>
#include <string_view>

// A foreground and a background, named as the pair they make. The values are the
// renderer table's indices and start at one.
enum class ColorPairId
{
	WHITE_BLACK = 1,
	WHITE_RED,
	WHITE_BLUE,
	WHITE_GREEN,
	BLACK_WHITE,
	BLACK_GREEN,
	BLACK_YELLOW,
	BLACK_RED,
	RED_BLACK,
	GREEN_BLACK,
	YELLOW_BLACK,
	BLUE_BLACK,
	CYAN_BLACK,
	MAGENTA_BLACK,
	CYAN_BLUE,
	RED_WHITE,
	GREEN_YELLOW,
	GREEN_MAGENTA,
	RED_YELLOW,
	GREEN_RED,
	BROWN_BLACK,
	DIM_GREEN_BLACK,
};

// Every pair, in the order the enum declares them, so a cycle through the editor's
// field reaches all of them and adding one is a single edit here.
inline constexpr std::array<ColorPairId, 22> ALL_COLOR_PAIR = {
	ColorPairId::WHITE_BLACK,
	ColorPairId::WHITE_RED,
	ColorPairId::WHITE_BLUE,
	ColorPairId::WHITE_GREEN,
	ColorPairId::BLACK_WHITE,
	ColorPairId::BLACK_GREEN,
	ColorPairId::BLACK_YELLOW,
	ColorPairId::BLACK_RED,
	ColorPairId::RED_BLACK,
	ColorPairId::GREEN_BLACK,
	ColorPairId::YELLOW_BLACK,
	ColorPairId::BLUE_BLACK,
	ColorPairId::CYAN_BLACK,
	ColorPairId::MAGENTA_BLACK,
	ColorPairId::CYAN_BLUE,
	ColorPairId::RED_WHITE,
	ColorPairId::GREEN_YELLOW,
	ColorPairId::GREEN_MAGENTA,
	ColorPairId::RED_YELLOW,
	ColorPairId::GREEN_RED,
	ColorPairId::BROWN_BLACK,
	ColorPairId::DIM_GREEN_BLACK,
};

// The list is the enum in order, each pair's value one past its position. Everything
// below derives from that, so a pair added to the enum and left out of the list - or
// given a value of its own - fails here rather than somewhere downstream.
static_assert(
	[]
	{
		for (std::size_t index = 0; index < ALL_COLOR_PAIR.size(); ++index)
		{
			if (ALL_COLOR_PAIR.at(index) != static_cast<ColorPairId>(index + 1))
			{
				return false;
			}
		}
		return true;
	}(),
	"ALL_COLOR_PAIR must list the enum in order, starting at one");

// Slots the renderer's table needs: one per pair, plus the unused zero the ids are
// offset past. Derived rather than written down, which is what let the old constant
// sit one ahead of a list nobody checked it against.
inline constexpr std::size_t COLOR_PAIR_TABLE_SIZE = ALL_COLOR_PAIR.size() + 1;

// Where a pair sits in the renderer's table.
//
// Example:
//   color_pair_index(ColorPairId::RED_BLACK);   // -> 9
[[nodiscard]] inline constexpr std::size_t color_pair_index(ColorPairId pair)
{
	return static_cast<std::size_t>(pair);
}

// The next pair in the enum's order, wrapping at the end, for the editor's field.
//
// Example:
//   next_color_pair(ColorPairId::WHITE_BLACK);      // -> ColorPairId::WHITE_RED
//   next_color_pair(ColorPairId::DIM_GREEN_BLACK);  // -> ColorPairId::WHITE_BLACK
[[nodiscard]] inline ColorPairId next_color_pair(ColorPairId pair)
{
	const auto found = std::ranges::find(ALL_COLOR_PAIR, pair);
	if (found == ALL_COLOR_PAIR.end() || found + 1 == ALL_COLOR_PAIR.end())
	{
		return ALL_COLOR_PAIR.front();
	}
	return *(found + 1);
}

// What a pair is called: the name a record carries for it and the one the editor
// shows. One table, so a record and a label cannot disagree.
//
// Deliberately without a default case, so adding a pair warns here under -Wswitch.
//
// Example:
//   color_pair_name(ColorPairId::YELLOW_BLACK);   // -> "yellow_black"
[[nodiscard]] inline constexpr std::string_view color_pair_name(ColorPairId pair)
{
	switch (pair)
	{
	case ColorPairId::WHITE_BLACK:
	{
		return "white_black";
	}
	case ColorPairId::WHITE_RED:
	{
		return "white_red";
	}
	case ColorPairId::WHITE_BLUE:
	{
		return "white_blue";
	}
	case ColorPairId::WHITE_GREEN:
	{
		return "white_green";
	}
	case ColorPairId::BLACK_WHITE:
	{
		return "black_white";
	}
	case ColorPairId::BLACK_GREEN:
	{
		return "black_green";
	}
	case ColorPairId::BLACK_YELLOW:
	{
		return "black_yellow";
	}
	case ColorPairId::BLACK_RED:
	{
		return "black_red";
	}
	case ColorPairId::RED_BLACK:
	{
		return "red_black";
	}
	case ColorPairId::GREEN_BLACK:
	{
		return "green_black";
	}
	case ColorPairId::YELLOW_BLACK:
	{
		return "yellow_black";
	}
	case ColorPairId::BLUE_BLACK:
	{
		return "blue_black";
	}
	case ColorPairId::CYAN_BLACK:
	{
		return "cyan_black";
	}
	case ColorPairId::MAGENTA_BLACK:
	{
		return "magenta_black";
	}
	case ColorPairId::CYAN_BLUE:
	{
		return "cyan_blue";
	}
	case ColorPairId::RED_WHITE:
	{
		return "red_white";
	}
	case ColorPairId::GREEN_YELLOW:
	{
		return "green_yellow";
	}
	case ColorPairId::GREEN_MAGENTA:
	{
		return "green_magenta";
	}
	case ColorPairId::RED_YELLOW:
	{
		return "red_yellow";
	}
	case ColorPairId::GREEN_RED:
	{
		return "green_red";
	}
	case ColorPairId::BROWN_BLACK:
	{
		return "brown_black";
	}
	case ColorPairId::DIM_GREEN_BLACK:
	{
		return "dim_green_black";
	}
	}

	return "white_black";
}

// The pair a record names, the inverse of color_pair_name. Throws naming what it
// read, so a record written by a build that knew a pair this one does not is refused
// rather than becoming whichever pair the number lands on.
//
// Example:
//   parse_color_pair("cyan_blue");   // -> ColorPairId::CYAN_BLUE
//   parse_color_pair("puce");        // throws std::runtime_error
[[nodiscard]] inline ColorPairId parse_color_pair(std::string_view name)
{
	const auto named = [name](ColorPairId pair)
	{
		return color_pair_name(pair) == name;
	};
	const auto found = std::ranges::find_if(ALL_COLOR_PAIR, named);
	if (found == ALL_COLOR_PAIR.end())
	{
		throw std::runtime_error(std::format("unknown color pair '{}'", name));
	}
	return *found;
}

// end of file: Colors.h
