#pragma once

#include <cstdint>
#include <optional>

class Creature;
struct GameContext;

class ConstitutionTracker
{
private:
    // The score whose bonus the owner's hit points already include. Empty until
    // the first application, so a fresh creature is never compared against a
    // score it did not have.
    std::optional<int> lastConstitution{};

    [[nodiscard]] int calculate_level_multiplier(const Creature& owner) const;

public:
    ConstitutionTracker() = default;
    ~ConstitutionTracker() = default;
    ConstitutionTracker(const ConstitutionTracker&) = delete;
    ConstitutionTracker(ConstitutionTracker&&) = delete;
    ConstitutionTracker& operator=(const ConstitutionTracker&) = delete;
    ConstitutionTracker& operator=(ConstitutionTracker&&) = delete;

    [[nodiscard]] std::optional<int> get_last_constitution() const noexcept { return lastConstitution; }
    void set_last_constitution(int value) noexcept { lastConstitution = value; }

    struct ConstitutionChangeResult
    {
        bool died{false};
        int hpDifference{0};
        int oldBonus{0};
        int newBonus{0};
        // True when the bonus was applied for the first time rather than moved:
        // there was no earlier score, so there is no change to report.
        bool firstApplication{false};
    };

    [[nodiscard]] ConstitutionChangeResult apply_constitution_changes(
        Creature& owner,
        GameContext& ctx);
};
