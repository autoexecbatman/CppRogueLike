#pragma once

#include <array>

#include "../Persistent/Persistent.h"

class Creature; // for no circular dependency with Creature.h
struct GameContext; // for dependency injection

//==AI==
class Ai : public Persistent
{
private:
	// Helper template for XP calculation
    template<std::size_t N>
    static constexpr int calculate_xp_for_level(int level, const std::array<int, N>& xp_table, int linear_progression) noexcept
    {
        // Defensive: clamp to valid range
        // Level should never be negative in normal gameplay
        if (level <= 0) return xp_table[0];

        // Check if the requested level is within the lookup table range
        // For levels 0-10, we have exact XP values defined in the table
        if (level < static_cast<int>(xp_table.size()))
		{
            return xp_table[level];
        }
        else
        {
            // For levels beyond the table (11+), calculate XP using linear progression
            // Start from the last table entry and add (linear_progression * levels beyond table)
            // Example: Level 12 fighter = 750000 (level 10) + (12 - 10) * 250000 = 1,250,000 XP
            const int max_level = static_cast<int>(xp_table.size()) - 1;
            return xp_table[max_level] + (level - max_level) * linear_progression;
        }
    }

public:
	virtual ~Ai() noexcept = default;
    Ai(const Ai&) = delete;
    Ai& operator=(const Ai&) = delete;
    Ai(Ai&&) noexcept = delete;
    Ai& operator=(Ai&&) noexcept = delete;

	virtual void update(Creature& owner, GameContext& ctx) = 0;

	// Type-safe hostility check - replaces dynamic_cast usage
	[[nodiscard]] virtual bool is_hostile() const { return true; } // Most AI types are hostile by default

	[[nodiscard]] static std::unique_ptr<Ai> create(const json& j);

	[[nodiscard]] int calculate_step(int positionDifference); // utility
	[[nodiscard]] int get_next_level_xp(GameContext& ctx, Creature& owner);
	void levelup_update(GameContext& ctx, Creature& owner);
    [[nodiscard]] static constexpr int calculate_fighter_xp(int level) noexcept;
    [[nodiscard]] static constexpr int calculate_rogue_xp(int level) noexcept;
    [[nodiscard]] static constexpr int calculate_cleric_xp(int level) noexcept;
    [[nodiscard]] static constexpr int calculate_wizard_xp(int level) noexcept;
protected:
    // Protected default constructor - only derived classes can use
    Ai() = default;

	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER, SHOPKEEPER
	};
};
