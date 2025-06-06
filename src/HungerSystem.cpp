#include "HungerSystem.h"
#include "Game.h"
#include "Colors/Colors.h"
#include <algorithm>

HungerSystem::HungerSystem() :
    hunger_value(0),
    hunger_max(1000),
    WELL_FED_THRESHOLD(200),
    SATIATED_THRESHOLD(400),
    HUNGRY_THRESHOLD(700),
    STARVING_THRESHOLD(900),
    DYING_THRESHOLD(950),
    current_state(HungerState::SATIATED)
{
}

void HungerSystem::increase_hunger(int amount) {
    hunger_value = std::min(hunger_value + amount, hunger_max);
    update_hunger_state();
}

void HungerSystem::decrease_hunger(int amount) {
    hunger_value = std::max(hunger_value - amount, 0);
    update_hunger_state();
}

HungerState HungerSystem::get_hunger_state() const {
    return current_state;
}

std::string HungerSystem::get_hunger_state_string() const {
    switch (current_state) {
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

int HungerSystem::get_hunger_value() const {
    return hunger_value;
}

int HungerSystem::get_hunger_color() const {
    switch (current_state) {
    case HungerState::WELL_FED:
        return HPBARFULL_PAIR;  // Green
    case HungerState::SATIATED:
        return WHITE_PAIR;       // White
    case HungerState::HUNGRY:
        return TROLL_PAIR;       // Yellow
    case HungerState::STARVING:
        return ORC_PAIR;         // Orange/Brown
    case HungerState::DYING:
        return HPBARMISSING_PAIR; // Red
    default:
        return WHITE_PAIR;
    }
}

bool HungerSystem::is_suffering_hunger_penalties() const {
    return current_state == HungerState::HUNGRY ||
        current_state == HungerState::STARVING ||
        current_state == HungerState::DYING;
}

void HungerSystem::apply_hunger_effects() {
    if (!game.player) return;

    // Reset any previous hunger effects first
    // This is assuming the player's base stats are stored somewhere and can be restored

    // Apply effects based on hunger state
    switch (current_state) {
    case HungerState::WELL_FED:
        // Bonuses for being well fed
        game.appendMessagePart(get_hunger_color(), "You feel strong and energetic!");
        game.finalizeMessage();
        // Potentially give bonus to strength or regen
        break;

    case HungerState::HUNGRY:
        // Minor penalties
        if (game.d.d10() == 1) {  // 10% chance each turn
            game.appendMessagePart(get_hunger_color(), "Your stomach growls.");
            game.finalizeMessage();
        }
        break;

    case HungerState::STARVING:
        // More severe penalties
        if (game.d.d6() == 1) {  // ~17% chance each turn
            game.appendMessagePart(get_hunger_color(), "You are weakened by hunger.");
            game.finalizeMessage();
            // Reduce player's strength temporarily
        }
        // Take small damage occasionally
        if (game.d.d20() == 1) {  // 5% chance each turn
            game.player->destructible->take_damage(*game.player, 1);
            game.appendMessagePart(get_hunger_color(), "You're starving!");
            game.finalizeMessage();
        }
        break;

    case HungerState::DYING:
        // Severe penalties, player is about to die
        game.appendMessagePart(get_hunger_color(), "You are dying from starvation!");
        game.finalizeMessage();
        // Take damage every turn
        game.player->destructible->take_damage(*game.player, 1);
        break;

    default:
        break;
    }
}

void HungerSystem::update_hunger_state() {
    HungerState old_state = current_state;

    if (hunger_value <= WELL_FED_THRESHOLD) {
        current_state = HungerState::WELL_FED;
    }
    else if (hunger_value <= SATIATED_THRESHOLD) {
        current_state = HungerState::SATIATED;
    }
    else if (hunger_value <= HUNGRY_THRESHOLD) {
        current_state = HungerState::HUNGRY;
    }
    else if (hunger_value <= STARVING_THRESHOLD) {
        current_state = HungerState::STARVING;
    }
    else {
        current_state = HungerState::DYING;
    }

    // Notify the player if hunger state has changed
    if (old_state != current_state) {
        game.appendMessagePart(get_hunger_color(), "You are now " + get_hunger_state_string() + ".");
        game.finalizeMessage();
    }
}