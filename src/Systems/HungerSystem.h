#pragma once

#include <nlohmann/json_fwd.hpp>
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

	// Returns current hunger state
	HungerState get_hunger_state() const;

	// Returns string representation of the current hunger state
	std::string get_hunger_state_string() const;

	// Returns hunger value
	int get_hunger_value() const;

	// Returns maximum hunger value
	int get_hunger_max() const;

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
	void load(GameContext& ctx, const nlohmann::json& j);

private:
	int hungerValue{ 0 }; // Internal hunger counter
	int hungerMax{ 1000 }; // Maximum hunger value
	bool wellFedMessageShown{ false }; // Prevents spam of well-fed message
	HungerState currentState{ HungerState::SATIATED };

	// Updates internal hunger state based on hunger value
	void update_hunger_state(GameContext& ctx);

};
