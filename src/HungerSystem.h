#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

#include "Colors.h"

struct GameContext;

// Hunger states in ascending order of hunger.
//
// The comments say what each state does, which is less than the names suggest: only
// the last two reach the player at all, and they do it with damage. Bonuses for being
// well fed and penalties short of damage are not implemented, and the enum used to
// promise both - see the issues directory for the design question of whether they
// should be.
enum class HungerState
{
	WELL_FED, // Says so once; no bonus
	SATIATED, // Nothing
	HUNGRY, // A flavour line on one turn in ten; no penalty
	STARVING, // A flavour line on one turn in six, and 1 damage on one in twenty
	DYING // A line and 1 damage every turn
};

class HungerSystem
{
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

public:
	HungerSystem() = default;
	~HungerSystem() = default;
	HungerSystem(const HungerSystem&) = delete;
	HungerSystem& operator=(const HungerSystem&) = delete;
	HungerSystem(HungerSystem&&) = delete;
	HungerSystem& operator=(HungerSystem&&) = delete;

	// Increases hunger by the specified amount
	void increase_hunger(GameContext& ctx, int amount);

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

	// Returns color code for hunger UI display
	ColorPairId get_hunger_color() const;

	// Returns true if player is hungry enough to suffer penalties
	bool is_suffering_hunger_penalties() const;

	// Apply hunger effects to player stats
	void apply_hunger_effects(GameContext& ctx);

	// Save/Load methods for game persistence
	void save(nlohmann::json& j) const;
	void load(const nlohmann::json& j);
};
