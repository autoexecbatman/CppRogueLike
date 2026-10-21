#include <algorithm>
#include <cassert>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "Colors.h"
#include "DamageInfo.h"
#include "GameContext.h"
#include "HungerSystem.h"
#include "MessageSystem.h"
#include "Player.h"
#include "RandomDice.h"

using json = nlohmann::json;

// Thresholds for different hunger states
constexpr int WELL_FED_THRESHOLD{ 200 };
constexpr int SATIATED_THRESHOLD{ 400 };
constexpr int HUNGRY_THRESHOLD{ 700 };
constexpr int STARVING_THRESHOLD{ 900 };

// The only place a counter becomes a state. Bounds ascend, so the first one that
// holds is the answer.
static HungerState hunger_state_for(int hungerValue)
{
	if (hungerValue <= WELL_FED_THRESHOLD)
	{
		return HungerState::WELL_FED;
	}

	if (hungerValue <= SATIATED_THRESHOLD)
	{
		return HungerState::SATIATED;
	}

	if (hungerValue <= HUNGRY_THRESHOLD)
	{
		return HungerState::HUNGRY;
	}

	if (hungerValue <= STARVING_THRESHOLD)
	{
		return HungerState::STARVING;
	}

	return HungerState::DYING;
}

void HungerSystem::begin_run()
{
	run = RunState{};
}

void HungerSystem::increase_hunger(GameContext& ctx, int amount)
{
	run.hungerValue = std::min(run.hungerValue + amount, run.hungerMax);
	notify_state_change(ctx);
}

void HungerSystem::decrease_hunger(GameContext& ctx, int amount)
{
	run.hungerValue = std::max(run.hungerValue - amount, 0);
	notify_state_change(ctx);
}

HungerState HungerSystem::get_hunger_state() const
{
	return hunger_state_for(run.hungerValue);
}

std::string HungerSystem::get_hunger_state_string() const
{
	switch (get_hunger_state())
	{

	case HungerState::WELL_FED:
	{
		return "Well Fed";
	}

	case HungerState::SATIATED:
	{
		return "Satiated";
	}

	case HungerState::HUNGRY:
	{
		return "Hungry";
	}

	case HungerState::STARVING:
	{
		return "Starving";
	}

	case HungerState::DYING:
	{
		return "Dying";
	}
	}
	// Every state returns above; a new one warns under -Wswitch rather than taking
	// a name that belongs to nothing.
	std::unreachable();
}

int HungerSystem::get_hunger_value() const
{
	return run.hungerValue;
}

int HungerSystem::get_hunger_max() const
{
	return run.hungerMax;
}

float HungerSystem::get_fullness_ratio() const
{
	if (run.hungerMax <= 0)
	{
		return 0.0f;
	}
	return std::clamp(1.0f - static_cast<float>(run.hungerValue) / static_cast<float>(run.hungerMax), 0.0f, 1.0f);
}

ColorPairId HungerSystem::get_hunger_color() const
{
	switch (get_hunger_state())
	{

	case HungerState::WELL_FED:
	{
		return ColorPairId::WHITE_GREEN; // Green
	}

	case HungerState::SATIATED:
	{
		return ColorPairId::WHITE_BLACK; // White
	}

	case HungerState::HUNGRY:
	{
		return ColorPairId::GREEN_BLACK;
	}

	case HungerState::STARVING:
	{
		return ColorPairId::RED_BLACK;
	}

	case HungerState::DYING:
	{
		return ColorPairId::WHITE_RED; // Red
	}
	}
	// Every state returns above; a new one warns under -Wswitch rather than taking
	// a colour that belongs to nothing.
	std::unreachable();
}

bool HungerSystem::is_suffering_hunger_penalties() const
{
	const HungerState state = get_hunger_state();
	return state == HungerState::HUNGRY ||
		state == HungerState::STARVING ||
		state == HungerState::DYING;
}

void HungerSystem::apply_hunger_effects(GameContext& ctx)
{
	assert(ctx.dice && "HungerSystem::apply_hunger_effects: called without dice");

	// The round upkeep is the only caller and it runs on a live game, so a missing
	// player is a wiring fault rather than a state the game passes through.
	assert(ctx.player() && "apply_hunger_effects ran without a player");

	switch (get_hunger_state())
	{

	case HungerState::SATIATED:
	{
		break;
	}

	case HungerState::WELL_FED:
	{
		// Said once per spell of being well fed, and reset when the state is left.
		if (!run.wellFedMessageShown)
		{
			ctx.messageSystem->append_message_part(get_hunger_color(), "You feel strong and energetic!");
			ctx.messageSystem->finalize_message();
			run.wellFedMessageShown = true;
		}
		break;
	}

	case HungerState::HUNGRY:
	{
		// Flavour only. Hunger costs the player nothing until STARVING.
		if (ctx.dice->d10() == 1)
		{ // 10% chance each turn
			ctx.messageSystem->append_message_part(get_hunger_color(), "Your stomach growls.");
			ctx.messageSystem->finalize_message();
		}
		break;
	}

	case HungerState::STARVING:
	{
		// Still flavour; the damage below is the first thing hunger actually costs.
		if (ctx.dice->d6() == 1)
		{ // ~17% chance each turn
			ctx.messageSystem->append_message_part(get_hunger_color(), "You are weakened by hunger.");
			ctx.messageSystem->finalize_message();
		}
		// One point on one turn in twenty, so starving is survivable for a while.
		if (ctx.dice->d20() == 1)
		{ // 5% chance each turn
			ctx.player()->take_damage_and_check_death(1, ctx, DamageType::PHYSICAL);
			ctx.messageSystem->append_message_part(get_hunger_color(), "You're starving!");
			ctx.messageSystem->finalize_message();
		}
		break;
	}

	case HungerState::DYING:
	{
		ctx.messageSystem->append_message_part(get_hunger_color(), "You are dying from starvation!");
		ctx.messageSystem->finalize_message();
		// Every turn now, with no roll to survive.
		ctx.player()->take_damage_and_check_death(1, ctx, DamageType::PHYSICAL);
		break;
	}
	}
}

void HungerSystem::notify_state_change(GameContext& ctx)
{
	const HungerState current = get_hunger_state();

	// The first call establishes what the player is; there is no move to report.
	if (!run.lastNotifiedState.has_value())
	{
		run.lastNotifiedState = current;
		return;
	}

	// Most ticks move the counter without crossing a threshold.
	if (*run.lastNotifiedState == current)
	{
		return;
	}

	const HungerState previous = *run.lastNotifiedState;

	// Say it before recording it: recording first and then throwing would silence this threshold forever.
	ctx.messageSystem->append_message_part(get_hunger_color(), "You are now " + get_hunger_state_string() + ".");
	ctx.messageSystem->finalize_message();

	run.lastNotifiedState = current;

	// Leaving well fed re-arms the message that fires on the way back into it.
	// The equality guard above already establishes that current is something else.
	if (previous == HungerState::WELL_FED)
	{
		run.wellFedMessageShown = false;
	}
}

void HungerSystem::save(json& output) const
{
	output["hungerValue"] = run.hungerValue;
	output["hungerMax"] = run.hungerMax;
	output["wellFedMessageShown"] = run.wellFedMessageShown;

	// Only what the player was told. The state itself is derived from hungerValue,
	// so storing it would be a second copy free to disagree with the counter.
	if (run.lastNotifiedState.has_value())
	{
		output["lastNotifiedState"] = static_cast<int>(*run.lastNotifiedState);
	}
}

void HungerSystem::load(const json& source)
{
	if (source.contains("hungerValue"))
	{
		run.hungerValue = source["hungerValue"];
	}
	if (source.contains("hungerMax"))
	{
		run.hungerMax = source["hungerMax"];
	}
	if (source.contains("wellFedMessageShown"))
	{
		run.wellFedMessageShown = source["wellFedMessageShown"];
	}

	// A save written before this field existed leaves it empty, and the first tick
	// after the load sets the baseline without announcing anything.
	if (source.contains("lastNotifiedState"))
	{
		run.lastNotifiedState = static_cast<HungerState>(source["lastNotifiedState"].get<int>());
	}
}
