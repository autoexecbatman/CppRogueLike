#include <algorithm>
#include <string>
#include <cassert>

#include <nlohmann/json.hpp>

#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Systems/MessageSystem.h"
#include "HungerSystem.h"

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

void HungerSystem::increase_hunger(GameContext& ctx, int amount)
{
	hungerValue = std::min(hungerValue + amount, hungerMax);
	notify_state_change(ctx);
}

void HungerSystem::decrease_hunger(GameContext& ctx, int amount)
{
	hungerValue = std::max(hungerValue - amount, 0);
	notify_state_change(ctx);
}

HungerState HungerSystem::get_hunger_state() const
{
	return hunger_state_for(hungerValue);
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

	default:
	{
		return "Unknown";
	}

	}
}

int HungerSystem::get_hunger_value() const
{
	return hungerValue;
}

int HungerSystem::get_hunger_max() const
{
	return hungerMax;
}

float HungerSystem::get_fullness_ratio() const
{
	if (hungerMax <= 0)
	{
		return 0.0f;
	}
	return std::clamp(1.0f - static_cast<float>(hungerValue) / static_cast<float>(hungerMax), 0.0f, 1.0f);
}

std::string HungerSystem::get_hunger_numerical_string() const
{
	return std::format("{}/{}", hungerValue, hungerMax);
}

std::string HungerSystem::get_hunger_bar_string(int bar_width) const
{
	// Calculate fill percentage
	float fill_percentage = static_cast<float>(hungerValue) / hungerMax;
	int filled_chars = static_cast<int>(fill_percentage * bar_width);

	std::string bar = "[";

	// Add filled portion
	for (int i = 0; i < filled_chars; ++i)
	{
		bar += "=";
	}

	// Add empty portion
	for (int i = filled_chars; i < bar_width; ++i)
	{
		bar += "-";
	}

	bar += "]";
	return bar;
}

int HungerSystem::get_hunger_color() const
{
	switch (get_hunger_state())
	{

	case HungerState::WELL_FED:
	{
		return WHITE_GREEN_PAIR; // Green
	}

	case HungerState::SATIATED:
	{
		return WHITE_BLACK_PAIR; // White
	}

	case HungerState::HUNGRY:
	{
		return GREEN_BLACK_PAIR; // Yellow
	}

	case HungerState::STARVING:
	{
		return RED_BLACK_PAIR; // Orange/Brown
	}

	case HungerState::DYING:
	{
		return WHITE_RED_PAIR; // Red
	}

	default:
	{
		return WHITE_BLACK_PAIR;
	}

	}
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
	if (!ctx.player())
	{
		return;
	}

	// Reset any previous hunger effects first
	// This is assuming the player's base stats are stored somewhere and can be restored

	// Apply effects based on hunger state
	switch (get_hunger_state())
	{

	case HungerState::SATIATED:
	{
		break;
	}

	case HungerState::WELL_FED:
	{
		// Bonuses for being well fed
		if (!wellFedMessageShown)
		{
			ctx.messageSystem->append_message_part(get_hunger_color(), "You feel strong and energetic!");
			ctx.messageSystem->finalize_message();
			wellFedMessageShown = true;
		}
		// Potentially give bonus to strength or regen
		break;
	}

	case HungerState::HUNGRY:
	{
		// Minor penalties
		if (ctx.dice->d10() == 1)
		{ // 10% chance each turn
			ctx.messageSystem->append_message_part(get_hunger_color(), "Your stomach growls.");
			ctx.messageSystem->finalize_message();
		}
		break;
	}

	case HungerState::STARVING:
	{
		// More severe penalties
		if (ctx.dice->d6() == 1)
		{ // ~17% chance each turn
			ctx.messageSystem->append_message_part(get_hunger_color(), "You are weakened by hunger.");
			ctx.messageSystem->finalize_message();
			// Reduce player's strength temporarily
		}
		// Take small damage occasionally
		if (ctx.dice->d20() == 1)
		{ // 5% chance each turn
			ctx.player()->take_damage_and_check_death(1, ctx);
			ctx.messageSystem->append_message_part(get_hunger_color(), "You're starving!");
			ctx.messageSystem->finalize_message();
		}
		break;
	}

	case HungerState::DYING:
	{
		// Severe penalties, player is about to die
		ctx.messageSystem->append_message_part(get_hunger_color(), "You are dying from starvation!");
		ctx.messageSystem->finalize_message();
		// Take damage every turn
		ctx.player()->take_damage_and_check_death(1, ctx);
		break;
	}

	}
}

void HungerSystem::notify_state_change(GameContext& ctx)
{
	const HungerState current = get_hunger_state();

	// The first call establishes what the player is; there is no move to report.
	if (!lastNotifiedState.has_value())
	{
		lastNotifiedState = current;
		return;
	}

	// Most ticks move the counter without crossing a threshold.
	if (*lastNotifiedState == current)
	{
		return;
	}

	const HungerState previous = *lastNotifiedState;

	// Say it before recording that it was said. Recording first and then throwing
	// on the way out would mark this threshold announced and silence it forever,
	// leaving a correct state and a missing message that no test can distinguish.
	ctx.messageSystem->append_message_part(get_hunger_color(), "You are now " + get_hunger_state_string() + ".");
	ctx.messageSystem->finalize_message();

	lastNotifiedState = current;

	// Leaving well fed re-arms the message that fires on the way back into it.
	// The equality guard above already establishes that current is something else.
	if (previous == HungerState::WELL_FED)
	{
		wellFedMessageShown = false;
	}
}

void HungerSystem::save(json& j) const
{
	j["hungerValue"] = hungerValue;
	j["hungerMax"] = hungerMax;
	j["wellFedMessageShown"] = wellFedMessageShown;

	// Only what the player was told. The state itself is derived from hungerValue,
	// so storing it would be a second copy free to disagree with the counter.
	if (lastNotifiedState.has_value())
	{
		j["lastNotifiedState"] = static_cast<int>(*lastNotifiedState);
	}
}

void HungerSystem::load(const json& j)
{
	if (j.contains("hungerValue"))
	{
		hungerValue = j["hungerValue"];
	}
	if (j.contains("hungerMax"))
	{
		hungerMax = j["hungerMax"];
	}
	if (j.contains("wellFedMessageShown"))
	{
		wellFedMessageShown = j["wellFedMessageShown"];
	}

	// A save written before this field existed leaves it empty, and the first tick
	// after the load sets the baseline without announcing anything.
	if (j.contains("lastNotifiedState"))
	{
		lastNotifiedState = static_cast<HungerState>(j["lastNotifiedState"].get<int>());
	}
}
