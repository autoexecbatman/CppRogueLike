#pragma once

#include <vector>

#include "BuffType.h"

struct GameContext;
class Creature;

// BuffSystem - Centralized buff management following system architecture pattern
class BuffSystem
{
public:
	// Buff lifecycle management
	void add_buff(Creature& creature, BuffType type, int value, int duration, bool is_set_effect) noexcept;
	void remove_buff(Creature& creature, BuffType type) noexcept;
	void update_creature_buffs(Creature& creature) noexcept;
	void restore_loaded_buff_states(Creature& creature) noexcept;

	// Query methods
	int get_buff_value(const Creature& creature, BuffType type) const noexcept;
	int get_buff_turns(const Creature& creature, BuffType type) const noexcept;
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
	std::vector<BuffType> remove_buffs_broken_by_attacking(Creature& creature) noexcept;
};
