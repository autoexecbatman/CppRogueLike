#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BuffSystem.h"
#include "BuffType.h"
#include "Creature.h"
#include "SavingThrow.h"
#include "TurnSchedule.h"

// The state each buff imposes. A creature's copy of these states is written only by
// BuffSystem::sync_states, so two buffs granting one state cannot clear each other.
static const std::unordered_map<BuffType, ActorState> buff_state_effects = {
	{ BuffType::INVISIBILITY, ActorState::IS_INVISIBLE },
	{ BuffType::HIDDEN_IN_SHADOWS, ActorState::IS_INVISIBLE },
	{ BuffType::ITEM_INVISIBILITY, ActorState::IS_INVISIBLE },
	{ BuffType::SLEEP, ActorState::IS_SLEEPING },
	{ BuffType::HOLD_PERSON, ActorState::IS_HELD },
	{ BuffType::SILENCE, ActorState::IS_SILENCED },
	{ BuffType::WEBBED, ActorState::IS_HELD },
};

// OCP: Data-driven AC calculation - BuffTypes that affect armor class
// AD&D 2e: Lower AC = better defense, so buff values are negated when calculating AC
static const std::unordered_set<BuffType> ac_affecting_buffs = {
	BuffType::SHIELD,
	BuffType::HAIRY_SPIDER_VENOM,
	// Future extensions: BuffType::ARMOR, BuffType::PROTECTION_FROM_EVIL, etc.
};

// OCP: Data-driven combat mechanics - Buffs that break when attacking
// AD&D 2e: Invisibility ends when you attack
static const std::unordered_set<BuffType> buffs_broken_by_attacking = {
	BuffType::INVISIBILITY,
	BuffType::SANCTUARY,
	BuffType::HIDDEN_IN_SHADOWS,
	BuffType::ITEM_INVISIBILITY,
};

// OCP: Data-driven hit calculation - BuffTypes that modify to-hit rolls
// AD&D 2e: Bless gives +1 to hit, future buffs may give other bonuses
static const std::unordered_map<BuffType, int> buff_hit_modifiers = {
	{ BuffType::BLESS, 1 },
	{ BuffType::HAIRY_SPIDER_VENOM, -HAIRY_SPIDER_VENOM_PENALTY },
	// Future extensions: {BuffType::PRAYER, 1}, {BuffType::CURSE, -1}, etc.
};

bool BuffSystem::is_buff_imposed(ActorState state) noexcept
{
	const auto imposes_state = [state](const auto& entry)
	{
		return entry.second == state;
	};
	return std::ranges::any_of(buff_state_effects, imposes_state);
}

void BuffSystem::sync_states(Creature& creature) noexcept
{
	for (const auto& [buffType, state] : buff_state_effects)
	{
		auto grants_state = [state](const Buff& buff)
		{
			const auto effect = buff_state_effects.find(buff.type);
			return effect != buff_state_effects.end() && effect->second == state;
		};

		// A state two buffs map to is visited twice; both visits give the same answer.
		if (std::ranges::any_of(creature.activeBuffs, grants_state))
		{
			if (!creature.has_state(state))
			{
				creature.add_state(state);
			}
		}
		else
		{
			creature.remove_state(state);
		}
	}
}

// The later of two endings, where none is an ending that never comes.
//
// Example:
//   later_ending(360, 960);            // -> 960
//   later_ending(360, std::nullopt);   // -> nullopt, lasting wins
static std::optional<int> later_ending(std::optional<int> first, std::optional<int> second) noexcept
{
	if (!first.has_value() || !second.has_value())
	{
		return std::nullopt;
	}
	return std::max(*first, *second);
}

// Puts one casting on the creature, or folds it into the one of its type already there.
//
// Example:
//   grant_buff(cleric, BuffType::BLESS, 0, 360, BuffApplication::ADD);            // blessed until 360
//   grant_buff(cleric, BuffType::BLESS, 0, std::nullopt, BuffApplication::ADD);   // now until broken
static void grant_buff(Creature& creature, BuffType type, int value, std::optional<int> expiry, BuffApplication application) noexcept
{
	// Find if buff already exists
	auto matches_type = [type](const Buff& b)
	{
		return b.type == type;
	};
	auto it = std::ranges::find_if(creature.activeBuffs, matches_type);

	if (it != creature.activeBuffs.end())
	{
		// A save is good against "that casting of the spell" (Player's Handbook, PDF page
		// 436), so a casting over a running one is met with fresh saves.
		it->opponentSaves.clear();

		// AD&D 2e: Same buff type - take highest value, extend duration
		if (value > it->value)
		{
			it->value = value; // Better buff, update value
			it->expiryTime = expiry; // The new casting's ending replaces the old one
			it->application = application;
		}
		else
		{
			// Weaker or equal buff, so it can only make the one already running last longer
			it->expiryTime = later_ending(it->expiryTime, expiry);
		}
	}
	else
	{
		// Add new buff
		Buff newBuff{};
		newBuff.type = type;
		newBuff.value = value;
		newBuff.expiryTime = expiry;
		newBuff.application = application;

		creature.activeBuffs.push_back(newBuff);
	}

	BuffSystem::sync_states(creature);
}

void BuffSystem::add_buff(Creature& creature, BuffType type, int value, int durationRounds, BuffApplication application, int currentTime) noexcept
{
	// When this casting runs out, worked once and stored, so nothing counts it down.
	grant_buff(creature, type, value, expiry_time(currentTime, durationRounds), application);
}

void BuffSystem::add_buff_until_broken(Creature& creature, BuffType type) noexcept
{
	grant_buff(creature, type, 0, std::nullopt, BuffApplication::ADD);
}

void BuffSystem::remove_buff(Creature& creature, BuffType type) noexcept
{
	// Remove buff - no stat restoration needed, getters calculate on the fly
	auto matches_type = [type](const Buff& b)
	{
		return b.type == type;
	};
	std::erase_if(creature.activeBuffs, matches_type);

	sync_states(creature);
}

void BuffSystem::update_creature_buffs(Creature& creature, int currentTime) noexcept
{
	// A buff the clock has reached or passed is over. Asked rather than counted, so the
	// answer is the same however many times a creature is driven in one round.
	auto is_expired = [currentTime](const Buff& b)
	{
		return b.expiryTime.has_value() && currentTime >= *b.expiryTime;
	};

	std::erase_if(creature.activeBuffs, is_expired);

	// Every buff left is live, loaded ones included, so the states are read from them now.
	sync_states(creature);
}

int BuffSystem::get_buff_value(const Creature& creature, BuffType type) const noexcept
{
	auto matches_type = [type](const Buff& b)
	{
		return b.type == type;
	};
	auto it = std::ranges::find_if(creature.activeBuffs, matches_type);
	return it != creature.activeBuffs.end() ? it->value : 0;
}

int BuffSystem::get_buff_turns(const Creature& creature, BuffType type, int currentTime) const noexcept
{
	auto matches_type = [type](const Buff& b)
	{
		return b.type == type;
	};
	auto it = std::ranges::find_if(creature.activeBuffs, matches_type);
	if (it == creature.activeBuffs.end())
	{
		return 0;
	}
	assert(it->expiryTime.has_value() && "get_buff_turns: a buff lasting until broken has no rounds to count");
	return rounds_remaining(currentTime, *it->expiryTime);
}

bool BuffSystem::has_buff(const Creature& creature, BuffType type) const noexcept
{
	auto matches_type = [type](const Buff& b)
	{
		return b.type == type;
	};
	return std::ranges::any_of(creature.activeBuffs, matches_type);
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

	for (const auto& buff : creature.activeBuffs | std::views::filter(is_ac_affecting))
	{
		total -= buff.value;
	}

	return total;
}

// Returns the to-hit penalty the target's wards impose on this attacker.
//
// AD&D 2e page 277: Protection from Evil penalises evil attackers by -2. A
// non-evil attacker walks through the ward untouched, which is the whole point
// of the spell being keyed to alignment.
//
// Example:
//   calculate_ward_penalty(goblin, wardedCleric);  // -> -2, goblin is evil
//   calculate_ward_penalty(knight, wardedCleric);  // -> 0, knight is not
int BuffSystem::calculate_ward_penalty(const Creature& attacker, const Creature& target) const noexcept
{
	if (!attacker.is_evil())
	{
		return 0;
	}

	for (const auto& buff : target.activeBuffs)
	{
		if (buff.type == BuffType::PROTECTION_FROM_EVIL)
		{
			return buff.value;
		}
	}

	return 0;
}

bool BuffSystem::is_turned_away_by_sanctuary(const Creature& attacker, Creature& warded, GameContext& ctx)
{
	auto is_sanctuary = [](const Buff& buff)
	{
		return buff.type == BuffType::SANCTUARY;
	};
	const auto casting = std::ranges::find_if(warded.activeBuffs, is_sanctuary);
	if (casting == warded.activeBuffs.end())
	{
		return false;
	}

	// Already rolled against this casting: the result stands.
	auto is_this_attacker = [&attacker](const OpponentSave& save)
	{
		return save.opponent == attacker.uniqueId;
	};
	const auto recorded = std::ranges::find_if(casting->opponentSaves, is_this_attacker);
	if (recorded != casting->opponentSaves.end())
	{
		return !recorded->isMade;
	}

	// The first attempt rolls, once, and the casting keeps the result.
	const bool isMade = SavingThrows::is_made(attacker, SavingThrow::SPELL, 0, ctx);
	casting->opponentSaves.push_back(OpponentSave{ attacker.uniqueId, isMade });
	return !isMade;
}

bool BuffSystem::ignores_warded_creature(const Creature& attacker, const Creature& warded) const noexcept
{
	auto is_sanctuary = [](const Buff& buff)
	{
		return buff.type == BuffType::SANCTUARY;
	};
	const auto casting = std::ranges::find_if(warded.activeBuffs, is_sanctuary);
	if (casting == warded.activeBuffs.end())
	{
		return false;
	}

	auto is_this_attacker_failing = [&attacker](const OpponentSave& save)
	{
		return save.opponent == attacker.uniqueId && !save.isMade;
	};
	return std::ranges::any_of(casting->opponentSaves, is_this_attacker_failing);
}

int BuffSystem::calculate_hit_modifier(const Creature& creature) const noexcept
{
	// AD&D 2e: Sum all to-hit bonuses from active buffs
	// Example: Bless gives +1, Prayer gives +1, total = +2
	int total = 0;

	for (const auto& buff : creature.activeBuffs)
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
	for (const auto& buff : creature.activeBuffs | std::views::filter(is_broken_by_attack))
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
