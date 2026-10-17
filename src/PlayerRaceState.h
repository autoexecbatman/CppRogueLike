#pragma once

// file: PlayerRaceState.h
//
// The races a player character may be. It sits in its own header so that the
// character creation blueprint in GameContext.h can hold the race as this enum
// without including Player.h. The race's name and its ability modifiers are
// derived from it by race_display_name and racial_ability_modifiers in Player.h,
// and nothing stores either.
//
// Usage:
//
//   PlayerRaceState race = PlayerRaceState::HALFLING;   // what the race menu writes
//   race_display_name(race);                             // -> "Halfling"
//   racial_ability_modifiers(race);                      // -> { -1, 1, 0, 0, 0, 0 }

enum class PlayerRaceState
{
	NONE,
	HUMAN,
	ELF,
	DWARF,
	HALFLING,
	GNOME,
	HALFELF
};

// end of file: PlayerRaceState.h
