#pragma once

#include <string>

// Hunger states in ascending order of hunger
enum class HungerState {
    WELL_FED,  // Recently ate, receive bonuses
    SATIATED,  // Normal state, no effects
    HUNGRY,    // Beginning to get hungry, minor penalties
    STARVING,  // Very hungry, major penalties
    DYING      // About to die from starvation
};

class HungerSystem {
public:
    HungerSystem();

    // Increases hunger by the specified amount (or default amount)
    void increase_hunger(int amount = 1);

    // Decreases hunger by the specified amount
    void decrease_hunger(int amount);

    // Returns current hunger state
    HungerState get_hunger_state() const;

    // Returns string representation of the current hunger state
    std::string get_hunger_state_string() const;

    // Returns hunger value
    int get_hunger_value() const;

    // Returns color code for hunger UI display
    int get_hunger_color() const;

    // Returns true if player is hungry enough to suffer penalties
    bool is_suffering_hunger_penalties() const;

    // Apply hunger effects to player stats
    void apply_hunger_effects();

private:
    int hunger_value;  // Internal hunger counter
    int hunger_max;    // Maximum hunger value

    // Thresholds for different hunger states
    const int WELL_FED_THRESHOLD;
    const int SATIATED_THRESHOLD;
    const int HUNGRY_THRESHOLD;
    const int STARVING_THRESHOLD;
    const int DYING_THRESHOLD;

    // Updates internal hunger state based on hunger value
    void update_hunger_state();

    HungerState current_state;
    bool well_fed_message_shown; // Prevents spam of well-fed message
};