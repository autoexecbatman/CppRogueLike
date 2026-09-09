#pragma once

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
