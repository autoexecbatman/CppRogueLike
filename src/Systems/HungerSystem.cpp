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

void HungerSystem::increase_hunger(GameContext& ctx, int amount)
{
	hungerValue = std::min(hungerValue + amount, hungerMax);
	update_hunger_state(ctx);
}

void HungerSystem::decrease_hunger(GameContext& ctx, int amount)
{
	hungerValue = std::max(hungerValue - amount, 0);
	update_hunger_state(ctx);
}

HungerState HungerSystem::get_hunger_state() const
{
	return currentState;
}

std::string HungerSystem::get_hunger_state_string() const
{
	switch (currentState)
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
}

int HungerSystem::get_hunger_value() const
{
	return hungerValue;
}

int HungerSystem::get_hunger_max() const
{
	return hungerMax;
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
	switch (currentState)
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
	return currentState == HungerState::HUNGRY ||
		currentState == HungerState::STARVING ||
		currentState == HungerState::DYING;
}

void HungerSystem::apply_hunger_effects(GameContext& ctx)
{
	if (!ctx.player)
	{
		return;
	}

	// Reset any previous hunger effects first
	// This is assuming the player's base stats are stored somewhere and can be restored

	// Apply effects based on hunger state
	switch (currentState)
	{

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
			ctx.player->destructible->take_damage(*ctx.player, 1, ctx);
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
		ctx.player->destructible->take_damage(*ctx.player, 1, ctx);
		break;
	}

	}
}

void HungerSystem::update_hunger_state(GameContext& ctx)
{
	HungerState oldState = currentState;

	if (hungerValue <= WELL_FED_THRESHOLD)
	{
		currentState = HungerState::WELL_FED;
	}
	else if (hungerValue <= SATIATED_THRESHOLD)
	{
		currentState = HungerState::SATIATED;
	}
	else if (hungerValue <= HUNGRY_THRESHOLD)
	{
		currentState = HungerState::HUNGRY;
	}
	else if (hungerValue <= STARVING_THRESHOLD)
	{
		currentState = HungerState::STARVING;
	}
	else
	{
		currentState = HungerState::DYING;
	}

	// Notify the player if hunger state has changed
	if (oldState != currentState)
	{
		ctx.messageSystem->append_message_part(get_hunger_color(), "You are now " + get_hunger_state_string() + ".");
		ctx.messageSystem->finalize_message();

		// Reset well-fed message flag when leaving well-fed state
		if (oldState == HungerState::WELL_FED && currentState != HungerState::WELL_FED)
		{
			wellFedMessageShown = false;
		}
	}
}

void HungerSystem::save(json& j) const
{
	j["hunger_value"] = hungerValue;
	j["hunger_max"] = hungerMax;
	j["current_state"] = static_cast<int>(currentState);
	j["well_fed_message_shown"] = wellFedMessageShown;
}

void HungerSystem::load(GameContext& ctx, const json& j)
{
	if (j.contains("hunger_value"))
	{
		hungerValue = j["hunger_value"];
	}
	if (j.contains("hunger_max"))
	{
		hungerMax = j["hunger_max"];
	}
	if (j.contains("current_state"))
	{
		currentState = static_cast<HungerState>(j["current_state"]);
	}
	if (j.contains("well_fed_message_shown"))
	{
		wellFedMessageShown = j["well_fed_message_shown"];
	}

	// Update hunger state based on loaded values to ensure consistency
	update_hunger_state(ctx);
}