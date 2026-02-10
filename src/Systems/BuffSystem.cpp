#include "BuffSystem.h"
#include "../Actor/Actor.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

// OCP: Data-driven buff state mapping - add new buffs here without modifying methods
static const std::unordered_map<BuffType, ActorState> buff_state_effects = {
	{BuffType::INVISIBILITY, ActorState::IS_INVISIBLE},
	// Future extensions: {BuffType::HOLD_PERSON, ActorState::IS_PARALYZED},
};

// OCP: Data-driven AC calculation - BuffTypes that affect armor class
// AD&D 2e: Lower AC = better defense, so buff values are negated when calculating AC
static const std::unordered_set<BuffType> ac_affecting_buffs = {
	BuffType::SHIELD,
	// Future extensions: BuffType::ARMOR, BuffType::PROTECTION_FROM_EVIL, etc.
};

// OCP: Data-driven combat mechanics - Buffs that break when attacking
// AD&D 2e: Invisibility ends when you attack
static const std::unordered_set<BuffType> buffs_broken_by_attacking = {
	BuffType::INVISIBILITY,
	// Future extensions: BuffType::SANCTUARY, etc.
};

// OCP: Data-driven hit calculation - BuffTypes that modify to-hit rolls
// AD&D 2e: Bless gives +1 to hit, future buffs may give other bonuses
static const std::unordered_map<BuffType, int> buff_hit_modifiers = {
	{BuffType::BLESS, 1},
	// Future extensions: {BuffType::PRAYER, 1}, {BuffType::CURSE, -1}, etc.
};

void BuffSystem::add_buff(Creature& creature, BuffType type, int value, int duration, bool is_set_effect) noexcept
{
	// Find if buff already exists
	auto matches_type = [type](const Buff& b) { return b.type == type; };
	auto it = std::ranges::find_if(creature.active_buffs, matches_type);

	if (it != creature.active_buffs.end())
	{
		// AD&D 2e: Same buff type - take highest value, extend duration
		if (value > it->value)
		{
			it->value = value;  // Better buff, update value
			it->turnsRemaining = duration;  // Reset duration
			it->is_set_effect = is_set_effect;  // Update effect type
		}
		else
		{
			// Weaker or equal buff, just extend duration
			it->turnsRemaining = std::max(it->turnsRemaining, duration);
		}
	}
	else
	{
		// Add new buff
		Buff newBuff{ };
		newBuff.type = type;
		newBuff.value = value;
		newBuff.turnsRemaining = duration;
		newBuff.is_set_effect = is_set_effect;

		// Apply state effects (data-driven, OCP compliant)
		if (buff_state_effects.contains(type))
		{
			creature.add_state(buff_state_effects.at(type));
		}

		creature.active_buffs.push_back(newBuff);
	}
}

void BuffSystem::remove_buff(Creature& creature, BuffType type) noexcept
{
	// Clear state effects (data-driven, OCP compliant)
	if (buff_state_effects.contains(type))
	{
		creature.remove_state(buff_state_effects.at(type));
	}

	// Remove buff - no stat restoration needed, getters calculate on the fly
	auto matches_type = [type](const Buff& b) { return b.type == type; };
	std::erase_if(creature.active_buffs, matches_type);
}

void BuffSystem::update_creature_buffs(Creature& creature) noexcept
{
	// Decrement all buff timers using ranges
	auto decrement_timer = [](Buff& b)
	{
		if (b.turnsRemaining > 0)
		{
			--b.turnsRemaining;
		}
	};
	std::ranges::for_each(creature.active_buffs, decrement_timer);

	// Clear states for all expiring buffs (data-driven, OCP compliant)
	for (const auto& [buffType, state] : buff_state_effects)
	{
		auto is_expiring_buff = [buffType](const Buff& b)
		{
			return b.type == buffType && b.turnsRemaining == 0;
		};
		auto expiring = creature.active_buffs | std::views::filter(is_expiring_buff);

		if (!std::ranges::empty(expiring))
		{
			creature.remove_state(state);
		}
	}

	// Remove expired buffs using std::erase_if (ranges-friendly)
	auto is_expired = [](const Buff& b) { return b.turnsRemaining == 0; };
	std::erase_if(creature.active_buffs, is_expired);
}

void BuffSystem::restore_loaded_buff_states(Creature& creature) noexcept
{
	// Restore state effects for all active buffs (idempotent - called after deserialization)
	for (const auto& buff : creature.active_buffs)
	{
		if (buff.turnsRemaining > 0)
		{
			if (buff_state_effects.contains(buff.type))
			{
				creature.add_state(buff_state_effects.at(buff.type));
			}
		}
	}
}

int BuffSystem::get_buff_value(const Creature& creature, BuffType type) const noexcept
{
	auto matches_type = [type](const Buff& b) { return b.type == type; };
	auto it = std::ranges::find_if(creature.active_buffs, matches_type);
	return it != creature.active_buffs.end() ? it->value : 0;
}

int BuffSystem::get_buff_turns(const Creature& creature, BuffType type) const noexcept
{
	auto matches_type = [type](const Buff& b) { return b.type == type; };
	auto it = std::ranges::find_if(creature.active_buffs, matches_type);
	return it != creature.active_buffs.end() ? it->turnsRemaining : 0;
}

bool BuffSystem::has_buff(const Creature& creature, BuffType type) const noexcept
{
	auto matches_type = [type](const Buff& b) { return b.type == type; };
	return std::ranges::find_if(creature.active_buffs, matches_type) != creature.active_buffs.end();
}

int BuffSystem::calculate_ac_bonus(const Creature& creature) const noexcept
{
	// AD&D 2e: Lower AC = better defense, so we negate buff values
	// Example: Shield spell with value=4 contributes -4 to AC (4 points of protection)
	int total = 0;
	auto is_ac_affecting = [](const Buff& buff)
	{
		return ac_affecting_buffs.contains(buff.type);
	};

	for (const auto& buff : creature.active_buffs | std::views::filter(is_ac_affecting))
	{
		total -= buff.value;
	}

	return total;
}

int BuffSystem::calculate_hit_modifier(const Creature& creature) const noexcept
{
	// AD&D 2e: Sum all to-hit bonuses from active buffs
	// Example: Bless gives +1, Prayer gives +1, total = +2
	int total = 0;

	for (const auto& buff : creature.active_buffs)
	{
		if (buff_hit_modifiers.contains(buff.type))
		{
			total += buff_hit_modifiers.at(buff.type);
		}
	}

	return total;
}

std::vector<BuffType> BuffSystem::remove_buffs_broken_by_attacking(Creature& creature) noexcept
{
	// AD&D 2e: Remove all buffs that break when attacking (e.g., Invisibility, Sanctuary)
	auto is_broken_by_attack = [](const Buff& buff)
	{
		return buffs_broken_by_attacking.contains(buff.type);
	};

	// Collect buffs to remove first to avoid iterator invalidation
	std::vector<BuffType> buffs_to_remove;
	for (const auto& buff : creature.active_buffs | std::views::filter(is_broken_by_attack))
	{
		buffs_to_remove.push_back(buff.type);
	}

	// Remove each buff (handles state cleanup)
	for (BuffType type : buffs_to_remove)
	{
		remove_buff(creature, type);
	}

	return buffs_to_remove;
}
