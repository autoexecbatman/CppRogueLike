// file: MonsterSpellcasting.cpp
#include <cassert>
#include <format>
#include <string>
#include <string_view>

#include "Actor.h"
#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "MessageSystem.h"
#include "MonsterSpellcasting.h"
#include "SpellRegistry.h"
#include "SpellSystem.h"
#include "Spellcasting.h"

namespace
{

// "The lich casts Magic Missile!", before the spell's own lines.
void announce_cast(const Creature& caster, std::string_view key, GameContext& ctx)
{
	ctx.messageSystem->message(caster.actorData.color, caster.actorData.name, MessageCompletion::CONTINUED);
	ctx.messageSystem->message(ColorPairId::WHITE_RED, std::format(" casts {}!", ctx.spellRegistry->get_by_key(key).name), MessageCompletion::FINISHED);
}

} // namespace

MonsterCast MonsterSpellcasting::try_cast_at(Creature& caster, Creature& target, GameContext& ctx)
{
	assert(ctx.spellRegistry && "MonsterSpellcasting::try_cast_at: called without a spellRegistry");
	assert(ctx.gameState && "MonsterSpellcasting::try_cast_at: called without a gameState");
	assert(ctx.messageSystem && "MonsterSpellcasting::try_cast_at: called without a messageSystem");

	// Silence stops every spell, so a silenced monster acts as one with none.
	if (caster.has_state(ActorState::IS_SILENCED))
	{
		return MonsterCast::NOTHING_CAST;
	}

	const auto can_aim = [&caster, &target, &ctx](std::string_view key)
	{
		return SpellSystem::can_aim_at(ctx.spellRegistry->get_by_key(key).effect_type, caster, target, ctx);
	};

	for (const std::string& memorized : caster.spellcasting.memorizedSpells)
	{
		if (!can_aim(memorized))
		{
			continue;
		}
		// A copy, because spending the spell erases the entry this loop is reading.
		const std::string key = memorized;
		const auto spend_the_copy = [&caster, key]([[maybe_unused]] GameContext& spentCtx)
		{
			spend_memorized(caster.spellcasting, key);
		};
		announce_cast(caster, key, ctx);
		SpellSystem::cast_spell_at(key, caster, SpellSource::MEMORIZED, target, spend_the_copy, ctx);
		return MonsterCast::CAST;
	}

	const int now = ctx.gameState->get_time();
	for (InnatePower& power : caster.spellcasting.innatePowers)
	{
		if (!is_power_ready(power, now) || !can_aim(power.spell))
		{
			continue;
		}
		const auto record_the_use = [&power, now]([[maybe_unused]] GameContext& usedCtx)
		{
			record_power_use(power, now);
		};
		announce_cast(caster, power.spell, ctx);
		SpellSystem::cast_spell_at(power.spell, caster, SpellSource::INNATE, target, record_the_use, ctx);
		return MonsterCast::CAST;
	}

	return MonsterCast::NOTHING_CAST;
}

// end of file: MonsterSpellcasting.cpp
