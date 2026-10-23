#pragma once

// file: CorpseEdibility.h
//
// Whether a monster's corpse can be eaten. A record states it as "corpse_edible", the
// creature carries it, and the corpse it leaves carries it to whoever tries to eat it.
// An inedible corpse is still a body on the floor; eating it is refused with its line.
//
// Usage:
//
//   creature.set_corpse("dead wraith", 0, "There is nothing left of it to eat.", CorpseEdibility::INEDIBLE);
//   corpse_edibility_from_record(true);    // -> CorpseEdibility::EDIBLE, what "corpse_edible": true reads as
//   is_corpse_edible(CorpseEdibility::INEDIBLE);  // -> false, what a save writes

enum class CorpseEdibility
{
	EDIBLE,
	INEDIBLE
};

// The edibility a record's or a save's "edible" bool names.
//
// Example:
//   corpse_edibility_from_record(false);  // -> CorpseEdibility::INEDIBLE
[[nodiscard]] inline constexpr CorpseEdibility corpse_edibility_from_record(
	bool isEdible) noexcept
{
	return isEdible ? CorpseEdibility::EDIBLE : CorpseEdibility::INEDIBLE;
}

// The bool a record or a save writes for an edibility.
//
// Example:
//   is_corpse_edible(CorpseEdibility::EDIBLE);  // -> true
[[nodiscard]] inline constexpr bool is_corpse_edible(
	CorpseEdibility edibility) noexcept
{
	return edibility == CorpseEdibility::EDIBLE;
}

// end of file: CorpseEdibility.h
