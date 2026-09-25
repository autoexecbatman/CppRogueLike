#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

// file: Alignment.h
//
// AD&D 2e alignment, as the two independent axes the rules actually use, plus
// the predicates that read them. Kept apart from Creature so a factory or an
// editor can take an alignment without including the whole creature.
//
// The book treats alignment as one label ("chaotic evil") but it is two
// independent choices, and alignment-keyed effects test one axis at a time:
// Protection from Evil asks only about morality, class restrictions such as the
// druid's "must be neutral" ask only about ethics. Nine combined values could
// answer neither without a lookup.
//
// Usage:
//
//   Creature goblin{ position, data };
//   goblin.set_ethics(Ethics::LAWFUL);     // goblins are lawful evil in 2e
//   goblin.set_morality(Morality::EVIL);
//   goblin.is_evil();                      // -> true, what Protection from Evil tests

// Law/chaos axis.
enum class Ethics
{
	LAWFUL,
	NEUTRAL,
	CHAOTIC,
};

// Good/evil axis.
enum class Morality
{
	GOOD,
	NEUTRAL,
	EVIL,
};

// The name a save carries for a creature's place on the law/chaos axis.
//
// Deliberately without a default case, so adding an Ethics value warns here and in the
// parser below under -Wswitch. No build passes -Werror, so it is a warning rather than
// a refusal.
//
// Example:
//   encode_ethics(Ethics::LAWFUL);  // -> "lawful"
[[nodiscard]] inline constexpr std::string_view encode_ethics(Ethics ethics)
{
	switch (ethics)
	{
	case Ethics::LAWFUL:
	{
		return "lawful";
	}
	case Ethics::NEUTRAL:
	{
		return "neutral";
	}
	case Ethics::CHAOTIC:
	{
		return "chaotic";
	}
	}

	return "neutral";
}

// The ethics a record names. Throws naming what it read.
//
// Example:
//   parse_ethics("chaotic");     // -> Ethics::CHAOTIC
//   parse_ethics("scrupulous");  // throws std::runtime_error
[[nodiscard]] inline Ethics parse_ethics(std::string_view name)
{
	if (name == "lawful")
	{
		return Ethics::LAWFUL;
	}
	if (name == "neutral")
	{
		return Ethics::NEUTRAL;
	}
	if (name == "chaotic")
	{
		return Ethics::CHAOTIC;
	}

	throw std::runtime_error(std::format("unknown ethics '{}'", name));
}

// The name a save carries for a creature's place on the good/evil axis.
//
// Deliberately without a default case, for the same reason as the encoder above.
//
// Example:
//   encode_morality(Morality::EVIL);  // -> "evil"
[[nodiscard]] inline constexpr std::string_view encode_morality(Morality morality)
{
	switch (morality)
	{
	case Morality::GOOD:
	{
		return "good";
	}
	case Morality::NEUTRAL:
	{
		return "neutral";
	}
	case Morality::EVIL:
	{
		return "evil";
	}
	}

	return "neutral";
}

// The morality a record names. Throws naming what it read.
//
// Example:
//   parse_morality("good");      // -> Morality::GOOD
//   parse_morality("saintly");   // throws std::runtime_error
[[nodiscard]] inline Morality parse_morality(std::string_view name)
{
	if (name == "good")
	{
		return Morality::GOOD;
	}
	if (name == "neutral")
	{
		return Morality::NEUTRAL;
	}
	if (name == "evil")
	{
		return Morality::EVIL;
	}

	throw std::runtime_error(std::format("unknown morality '{}'", name));
}

// Turns a creature's ethics one step toward chaotic, stopping at CHAOTIC.
//
// HOUSE RULE, not an AD&D 2e citation. The Player's Handbook states that
// alignment shifts through repeated deeds and that "several occasions of lax
// behavior are required", but gives no formula, no step size and no table -
// drift is left to the DM. One step per betrayal is ours.
//
// Example:
//   shift_toward_chaotic(Ethics::LAWFUL);  // -> Ethics::NEUTRAL
//   shift_toward_chaotic(Ethics::CHAOTIC); // -> Ethics::CHAOTIC, already there
[[nodiscard]] constexpr Ethics shift_toward_chaotic(Ethics ethics)
{
	switch (ethics)
	{
	case Ethics::LAWFUL:
	{
		return Ethics::NEUTRAL;
	}
	case Ethics::NEUTRAL:
	{
		return Ethics::CHAOTIC;
	}
	case Ethics::CHAOTIC:
	{
		return Ethics::CHAOTIC;
	}
	}
	return ethics;
}
