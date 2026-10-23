#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

// file: CreatureSize.h
//
// The six size categories every Monstrous Manual stat block prints on its Size line.
// A monster's size is data, authored in monsters.json and in the monster editor, and
// the Dungeon Master Guide's Ability Scores for Monsters reads a monster's Strength,
// Dexterity and Constitution off the column for its size (Tables 6 to 8). Kept apart
// from Creature so the registry and the editor can name a size without the creature.
//
// Usage:
//
//   const CreatureSize ogre = parse_creature_size("large"); // what monsters.json carries
//   encode_creature_size(ogre); // -> "large", what a save writes back
//   next_creature_size(ogre); // -> CreatureSize::HUGE_SIZE, the editor's next value

// T, S, M, L, H and G on the Size line: Tiny 2' or less, Small 2-4', Man-sized 4-7',
// Large 7-12', Huge 12-25', Gargantuan 25' and over.
enum class CreatureSize
{
	TINY,
	SMALL,
	MEDIUM,
	LARGE,
	// HUGE alone is a macro in the C library's <math.h> under Emscripten.
	HUGE_SIZE,
	GARGANTUAN
};

// Every CreatureSize, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<CreatureSize, 6> ALL_CREATURE_SIZE = {
	CreatureSize::TINY,
	CreatureSize::SMALL,
	CreatureSize::MEDIUM,
	CreatureSize::LARGE,
	CreatureSize::HUGE_SIZE,
	CreatureSize::GARGANTUAN
};

// The next CreatureSize in that order, wrapping at the end.
//
// Example:
//   next_creature_size(CreatureSize::TINY); // -> CreatureSize::SMALL
//   next_creature_size(CreatureSize::GARGANTUAN); // -> CreatureSize::TINY
[[nodiscard]] inline CreatureSize next_creature_size(CreatureSize value)
{
	const auto found = std::ranges::find(ALL_CREATURE_SIZE, value);
	if (found == ALL_CREATURE_SIZE.end() ||
		found + 1 == ALL_CREATURE_SIZE.end())
	{
		return ALL_CREATURE_SIZE.front();
	}
	return *(found + 1);
}

// The name monsters.json carries for a size.
//
// Example:
//   encode_creature_size(CreatureSize::HUGE_SIZE); // -> "huge"
[[nodiscard]] inline std::string_view encode_creature_size(CreatureSize size)
{
	switch (size)
	{

	case CreatureSize::TINY:
	{
		return "tiny";
	}

	case CreatureSize::SMALL:
	{
		return "small";
	}

	case CreatureSize::MEDIUM:
	{
		return "medium";
	}

	case CreatureSize::LARGE:
	{
		return "large";
	}

	case CreatureSize::HUGE_SIZE:
	{
		return "huge";
	}

	case CreatureSize::GARGANTUAN:
	{
		return "gargantuan";
	}
	}

	return "medium";
}

// The size a record names. Throws naming what it read.
//
// Example:
//   parse_creature_size("gargantuan"); // -> CreatureSize::GARGANTUAN
//   parse_creature_size("enormous"); // throws std::runtime_error
[[nodiscard]] inline CreatureSize parse_creature_size(std::string_view name)
{
	const auto named = [name](CreatureSize size)
	{
		return encode_creature_size(size) == name;
	};

	// Searched against the encoder's names, so the two cannot disagree.
	const auto found = std::ranges::find_if(ALL_CREATURE_SIZE, named);
	if (found == ALL_CREATURE_SIZE.end())
	{
		throw std::runtime_error(
			std::format("unknown creature size '{}'", name));
	}
	return *found;
}

// end of file: CreatureSize.h
