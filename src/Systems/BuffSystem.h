#pragma once

#include "../Actor/Actor.h"

struct GameContext;
class Creature;

// BuffSystem - Centralized buff management following system architecture pattern
class BuffSystem
{
public:
	BuffSystem() = default;
	~BuffSystem() = default;

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
	std::vector<BuffType> remove_buffs_broken_by_attacking(Creature& creature) noexcept;
};
