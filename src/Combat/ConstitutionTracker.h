#pragma once

#include <cstdint>

class Creature;
struct GameContext;

class ConstitutionTracker
{
private:
    int lastConstitution{};

    [[nodiscard]] int calculate_constitution_hp_bonus_for_value(int constitution, GameContext& ctx) const;
    [[nodiscard]] int calculate_level_multiplier(const Creature& owner) const;

public:
    ConstitutionTracker() = default;
    ~ConstitutionTracker() = default;
    ConstitutionTracker(const ConstitutionTracker&) = delete;
    ConstitutionTracker(ConstitutionTracker&&) = delete;
    ConstitutionTracker& operator=(const ConstitutionTracker&) = delete;
    ConstitutionTracker& operator=(ConstitutionTracker&&) = delete;

    [[nodiscard]] int get_last_constitution() const noexcept { return lastConstitution; }
    void set_last_constitution(int value) noexcept { lastConstitution = value; }

    struct ConstitutionChangeResult
    {
        bool died{false};
        int hpDifference{0};
        int oldBonus{0};
        int newBonus{0};
    };

    [[nodiscard]] ConstitutionChangeResult apply_constitution_changes(
        Creature& owner,
        GameContext& ctx);
};
