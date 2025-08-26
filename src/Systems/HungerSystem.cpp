#include "HungerSystem.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include <algorithm>

HungerSystem::HungerSystem() :
    hunger_value(0),
    hunger_max(1000),
    WELL_FED_THRESHOLD(200),
    SATIATED_THRESHOLD(400),
    HUNGRY_THRESHOLD(700),
    STARVING_THRESHOLD(900),
    DYING_THRESHOLD(950),
    current_state(HungerState::SATIATED),
    well_fed_message_shown(false)
{
}

void HungerSystem::increase_hunger(int amount)
{
    hunger_value = std::min(hunger_value + amount, hunger_max);
    update_hunger_state();
}

void HungerSystem::decrease_hunger(int amount)
{
    hunger_value = std::max(hunger_value - amount, 0);
    update_hunger_state();
}

HungerState HungerSystem::get_hunger_state() const
{
    return current_state;
}

std::string HungerSystem::get_hunger_state_string() const
{
    switch (current_state)
    {
    case HungerState::WELL_FED:
        return "Well Fed";
    case HungerState::SATIATED:
        return "Satiated";
    case HungerState::HUNGRY:
        return "Hungry";
    case HungerState::STARVING:
        return "Starving";
    case HungerState::DYING:
        return "Dying";
    default:
        return "Unknown";
    }
}

int HungerSystem::get_hunger_value() const
{
    return hunger_value;
}

int HungerSystem::get_hunger_max() const
{
    return hunger_max;
}

std::string HungerSystem::get_hunger_numerical_string() const
{
    return std::to_string(hunger_value) + "/" + std::to_string(hunger_max);
}

std::string HungerSystem::get_hunger_bar_string(int bar_width) const
{
    // Calculate fill percentage
    float fill_percentage = static_cast<float>(hunger_value) / hunger_max;
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
    switch (current_state) {
    case HungerState::WELL_FED:
        return WHITE_GREEN_PAIR;  // Green
    case HungerState::SATIATED:
        return WHITE_BLACK_PAIR;       // White
    case HungerState::HUNGRY:
        return GREEN_BLACK_PAIR;       // Yellow
    case HungerState::STARVING:
        return RED_BLACK_PAIR;         // Orange/Brown
    case HungerState::DYING:
        return WHITE_RED_PAIR; // Red
    default:
        return WHITE_BLACK_PAIR;
    }
}

bool HungerSystem::is_suffering_hunger_penalties() const
{
    return current_state == HungerState::HUNGRY ||
        current_state == HungerState::STARVING ||
        current_state == HungerState::DYING;
}

void HungerSystem::apply_hunger_effects()
{
    if (!game.player) return;

    // Reset any previous hunger effects first
    // This is assuming the player's base stats are stored somewhere and can be restored

    // Apply effects based on hunger state
    switch (current_state)
    {
    case HungerState::WELL_FED:
        // Bonuses for being well fed
        if (!well_fed_message_shown)
        {
            game.append_message_part(get_hunger_color(), "You feel strong and energetic!");
            game.finalize_message();
            well_fed_message_shown = true;
        }
        // Potentially give bonus to strength or regen
        break;

    case HungerState::HUNGRY:
        // Minor penalties
        if (game.d.d10() == 1)
        {  // 10% chance each turn
            game.append_message_part(get_hunger_color(), "Your stomach growls.");
            game.finalize_message();
        }
        break;

    case HungerState::STARVING:
        // More severe penalties
        if (game.d.d6() == 1)
        {  // ~17% chance each turn
            game.append_message_part(get_hunger_color(), "You are weakened by hunger.");
            game.finalize_message();
            // Reduce player's strength temporarily
        }
        // Take small damage occasionally
        if (game.d.d20() == 1)
        {  // 5% chance each turn
            game.player->destructible->take_damage(*game.player, 1);
            game.append_message_part(get_hunger_color(), "You're starving!");
            game.finalize_message();
        }
        break;

    case HungerState::DYING:
        // Severe penalties, player is about to die
        game.append_message_part(get_hunger_color(), "You are dying from starvation!");
        game.finalize_message();
        // Take damage every turn
        game.player->destructible->take_damage(*game.player, 1);
        break;

    default:
        break;
    }
}

void HungerSystem::update_hunger_state()
{
    HungerState old_state = current_state;

    if (hunger_value <= WELL_FED_THRESHOLD)
    {
        current_state = HungerState::WELL_FED;
    }
    else if (hunger_value <= SATIATED_THRESHOLD)
    {
        current_state = HungerState::SATIATED;
    }
    else if (hunger_value <= HUNGRY_THRESHOLD)
    {
        current_state = HungerState::HUNGRY;
    }
    else if (hunger_value <= STARVING_THRESHOLD)
    {
        current_state = HungerState::STARVING;
    }
    else {
        current_state = HungerState::DYING;
    }

    // Notify the player if hunger state has changed
    if (old_state != current_state)
    {
        game.append_message_part(get_hunger_color(), "You are now " + get_hunger_state_string() + ".");
        game.finalize_message();
        
        // Reset well-fed message flag when leaving well-fed state
        if (old_state == HungerState::WELL_FED && current_state != HungerState::WELL_FED)
        {
            well_fed_message_shown = false;
        }
    }
}

void HungerSystem::save(json& j) const
{
    j["hunger_value"] = hunger_value;
    j["hunger_max"] = hunger_max;
    j["current_state"] = static_cast<int>(current_state);
    j["well_fed_message_shown"] = well_fed_message_shown;
}

void HungerSystem::load(const json& j)
{
    if (j.contains("hunger_value"))
    {
        hunger_value = j["hunger_value"];
    }
    if (j.contains("hunger_max"))
    {
        hunger_max = j["hunger_max"];
    }
    if (j.contains("current_state"))
    {
        current_state = static_cast<HungerState>(j["current_state"]);
    }
    if (j.contains("well_fed_message_shown"))
    {
        well_fed_message_shown = j["well_fed_message_shown"];
    }
    
    // Update hunger state based on loaded values to ensure consistency
    update_hunger_state();
}