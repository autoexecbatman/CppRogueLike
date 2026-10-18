#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

// file: Attitude.h
//
// How a creature feels about the player. A monster's attitude is data, authored in
// monsters.json and in the monster editor; the shopkeeper is peaceful and every other
// monster hostile. Kept apart from Creature so the registry and the editor can name an
// attitude without the creature.
//
// Usage:
//
//   const Attitude merchant = parse_attitude("peaceful"); // what monsters.json carries
//   encode_attitude(merchant); // -> "peaceful", what a save writes back
//   next_attitude(merchant); // -> Attitude::FRIENDLY, the editor's next value
//   needs_attack_confirmation(merchant); // -> true

// Ordered from most hostile to most friendly, so threshold comparisons are meaningful.
//
// This is deliberately separate from whether the creature has noticed the player: a
// hostile creature that has not seen you is still hostile.
enum class Attitude
{
	HOSTILE, // attacks the player on sight
	PEACEFUL, // will not attack unless provoked
	FRIENDLY, // will not attack, and sides with the player
	TAME, // a companion under the player's direction
};

// Every Attitude, in the order the enum declares them, so a cycle through the editor's
// field reaches all of them and adding one is a single edit here.
inline constexpr std::array<Attitude, 4> ALL_ATTITUDE = {
	Attitude::HOSTILE,
	Attitude::PEACEFUL,
	Attitude::FRIENDLY,
	Attitude::TAME
};

// True when attacking this creature is a deliberate act the player should be
// asked to confirm.
//
// Example:
//   needs_attack_confirmation(Attitude::HOSTILE);  // -> false
//   needs_attack_confirmation(Attitude::PEACEFUL); // -> true
[[nodiscard]] constexpr bool needs_attack_confirmation(Attitude attitude)
{
	return attitude >= Attitude::PEACEFUL;
}

// The next Attitude in that order, wrapping at the end.
//
// Example:
//   next_attitude(Attitude::HOSTILE); // -> Attitude::PEACEFUL
//   next_attitude(Attitude::TAME); // -> Attitude::HOSTILE
[[nodiscard]] inline Attitude next_attitude(Attitude value)
{
	const auto found = std::ranges::find(ALL_ATTITUDE, value);
	if (found == ALL_ATTITUDE.end() || found + 1 == ALL_ATTITUDE.end())
	{
		return ALL_ATTITUDE.front();
	}
	return *(found + 1);
}

// The name monsters.json carries for an attitude.
//
// Example:
//   encode_attitude(Attitude::PEACEFUL); // -> "peaceful"
[[nodiscard]] inline std::string_view encode_attitude(Attitude attitude)
{
	switch (attitude)
	{
	case Attitude::HOSTILE:
	{
		return "hostile";
	}
	case Attitude::PEACEFUL:
	{
		return "peaceful";
	}
	case Attitude::FRIENDLY:
	{
		return "friendly";
	}
	case Attitude::TAME:
	{
		return "tame";
	}
	}

	return "hostile";
}

// The attitude a record names. Throws naming what it read.
//
// Example:
//   parse_attitude("tame"); // -> Attitude::TAME
//   parse_attitude("grumpy"); // throws std::runtime_error
[[nodiscard]] inline Attitude parse_attitude(std::string_view name)
{
	const auto named = [name](Attitude attitude)
	{
		return encode_attitude(attitude) == name;
	};

	// Searched against the encoder's names, so the two cannot disagree.
	const auto found = std::ranges::find_if(ALL_ATTITUDE, named);
	if (found == ALL_ATTITUDE.end())
	{
		throw std::runtime_error(std::format("unknown attitude '{}'", name));
	}
	return *found;
}

// end of file: Attitude.h
