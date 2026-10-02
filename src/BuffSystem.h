#pragma once

#include <vector>

#include "BuffType.h"

struct GameContext;
class Creature;

// BuffSystem - Centralized buff management following system architecture pattern
class BuffSystem
{
public:
	// Grants a buff for a duration the book prints in rounds, read from `currentTime` on
	// GameState's clock. A second casting of the same type keeps the better value and
	// the later ending, and gives every opponent a fresh save against it.
	//
	// Example, blessing a cleric for three rounds at the start of the game:
	//   add_buff(cleric, BuffType::BLESS, 0, 3, false, 0);
	//   get_buff_turns(cleric, BuffType::BLESS, 0);     // -> 3
	//   get_buff_turns(cleric, BuffType::BLESS, 240);   // -> 1, two rounds in
	void add_buff(Creature& creature, BuffType type, int value, int durationRounds, bool isSetEffect, int currentTime) noexcept;
	void remove_buff(Creature& creature, BuffType type) noexcept;

	// Retires whatever the clock has passed, clearing the states those buffs imposed.
	// Called at the top of every creature's update, which is once per action: it reads
	// the clock rather than counting, so being called twice in a round costs nothing.
	void update_creature_buffs(Creature& creature, int currentTime) noexcept;
	void restore_loaded_buff_states(Creature& creature, int currentTime) noexcept;

	// Query methods
	int get_buff_value(const Creature& creature, BuffType type) const noexcept;

	// Whole rounds the buff has left at `currentTime`, rounded up so that a buff still
	// in force never reports zero. A creature without the buff reads zero as well.
	int get_buff_turns(const Creature& creature, BuffType type, int currentTime) const noexcept;
	bool has_buff(const Creature& creature, BuffType type) const noexcept;

	// Combat calculations - data-driven, OCP compliant
	int calculate_ac_bonus(const Creature& creature) const noexcept;
	int calculate_hit_modifier(const Creature& creature) const noexcept;

	// Penalty an attacker suffers from the target's own wards. Reads both
	// creatures because the penalty depends on who is swinging: Protection
	// from Evil bites only evil attackers.
	int calculate_ward_penalty(const Creature& attacker, const Creature& target) const noexcept;

	// Whether the warded creature's Sanctuary turns this attacker away. PHB page 436:
	// an opponent attempting to attack saves against the spell; made, it "is
	// unaffected by that casting"; failed, it "totally ignores the warded creature
	// for the duration". The first ask rolls the save and records it on the casting;
	// every later ask reads the record. No Sanctuary, no save.
	//
	// Example, the goblin rolling a 1 against a fresh casting, the orc a 20:
	//   is_turned_away_by_sanctuary(goblin, player, ctx);   // -> true, and recorded
	//   is_turned_away_by_sanctuary(goblin, player, ctx);   // -> true, no roll
	//   is_turned_away_by_sanctuary(orc, player, ctx);      // -> false for this casting
	bool is_turned_away_by_sanctuary(const Creature& attacker, Creature& warded, GameContext& ctx);

	// Whether this attacker has already lost track of the warded creature: a Sanctuary
	// casting on `warded` holding a save this attacker failed. Asks the record and
	// nothing else - **it never rolls**, because the same page grants a save only to an
	// opponent "attempting to strike", and a creature deciding where to walk has not
	// swung yet. That is what lets the artificial intelligence read the ward without
	// spending a save that was never earned.
	//
	// Example, after the goblin failed its save and the orc made one:
	//   ignores_warded_creature(goblin, player);  // -> true, and no roll was made
	//   ignores_warded_creature(orc, player);     // -> false, it may still hunt
	//   ignores_warded_creature(kobold, player);  // -> false, it has never attacked
	bool ignores_warded_creature(const Creature& attacker, const Creature& warded) const noexcept;
	std::vector<BuffType> remove_buffs_broken_by_attacking(Creature& creature) noexcept;
};
