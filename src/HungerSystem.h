#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

struct GameContext;

// Hunger states in ascending order of hunger
enum class HungerState
{
	WELL_FED, // Recently ate, receive bonuses
	SATIATED, // Normal state, no effects
	HUNGRY, // Beginning to get hungry, minor penalties
	STARVING, // Very hungry, major penalties
	DYING // About to die from starvation
};

class HungerSystem
{
public:
	HungerSystem() = default;
	~HungerSystem() = default;
	HungerSystem(const HungerSystem&) = delete;
	HungerSystem& operator=(const HungerSystem&) = delete;
	HungerSystem(HungerSystem&&) = delete;
	HungerSystem& operator=(HungerSystem&&) = delete;

	// Increases hunger by the specified amount (or default amount)
	void increase_hunger(GameContext& ctx, int amount = 1);

	// Decreases hunger by the specified amount
	void decrease_hunger(GameContext& ctx, int amount);

	// The state the counter is in, derived on every call from hungerValue against
	// the threshold table. There is no stored copy of this to fall out of step
	// with the counter, so it is right before anything has ticked.
	//
	// Example (values from HungerSystemTest):
	//
	//   HungerSystem hunger{};                    // nothing eaten, counter at 0
	//   hunger.get_hunger_state();                // -> HungerState::WELL_FED
	//
	//   hunger.increase_hunger(ctx, 700);         // counter at 700
	//   hunger.get_hunger_state();                // -> HungerState::HUNGRY
	[[nodiscard]] HungerState get_hunger_state() const;

	// Returns string representation of the current hunger state
	std::string get_hunger_state_string() const;

	// Returns hunger value
	int get_hunger_value() const;

	// Returns maximum hunger value
	int get_hunger_max() const;

	// How full the creature is: 1.0 just after eating, 0.0 at starvation.
	// hungerValue counts up toward starving, so a meter drawn straight from it
	// empties as the creature fills. This is the complement, and it is what the
	// HUD draws, because it sits beside a health bar that fills when healthy.
	//
	// Example (values from HungerSystemTest, which pins all three):
	//
	//   HungerSystem hunger{};                              // nothing eaten yet
	//   hunger.get_fullness_ratio();                        // -> 1.0
	//
	//   hunger.increase_hunger(ctx, hunger.get_hunger_max() / 4);
	//   hunger.get_fullness_ratio();                        // -> 0.75
	//
	//   hunger.increase_hunger(ctx, hunger.get_hunger_max() * 2);
	//   hunger.get_fullness_ratio();                        // -> 0.0, never below
	[[nodiscard]] float get_fullness_ratio() const;

	// Returns numerical hunger display (e.g., "150/1000")
	std::string get_hunger_numerical_string() const;

	// Returns hunger progress bar string
	std::string get_hunger_bar_string(int bar_width = 20) const;

	// Returns color code for hunger UI display
	int get_hunger_color() const;

	// Returns true if player is hungry enough to suffer penalties
	bool is_suffering_hunger_penalties() const;

	// Apply hunger effects to player stats
	void apply_hunger_effects(GameContext& ctx);

	// Save/Load methods for game persistence
	void save(nlohmann::json& j) const;
	void load(const nlohmann::json& j);

private:
	int hungerValue{ 0 }; // Internal hunger counter
	int hungerMax{ 1000 }; // Maximum hunger value
	bool wellFedMessageShown{ false }; // Prevents spam of well-fed message

	// What the player was last told they were, empty until the first message.
	// Held only so a transition has something to compare against; the current
	// state comes from get_hunger_state().
	std::optional<HungerState> lastNotifiedState{};

	// Announces a move to a different state, and records what was announced.
	void notify_state_change(GameContext& ctx);

};
