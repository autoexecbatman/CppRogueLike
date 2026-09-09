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
