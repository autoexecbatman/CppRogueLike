#pragma once

#include <array>

class CalculatedTHAC0s
{
private:
    // AD&D 2e THAC0 progression tables (index 0 unused, levels 1-20)
    static constexpr std::array<int, 21> cleric_thac0 = {
        0,  // Level 0 (unused)
        20, // Level 1
        20, // Level 2
        20, // Level 3
        18, // Level 4
        18, // Level 5
        18, // Level 6
        16, // Level 7
        16, // Level 8
        16, // Level 9
        14, // Level 10
        14, // Level 11
        14, // Level 12
        12, // Level 13
        12, // Level 14
        12, // Level 15
        10, // Level 16
        10, // Level 17
        10, // Level 18
        8,  // Level 19
        8   // Level 20
    };

    static constexpr std::array<int, 21> rogue_thac0 = {
        0,  // Level 0 (unused)
        20, // Level 1
        20, // Level 2
        19, // Level 3
        19, // Level 4
        18, // Level 5
        18, // Level 6
        17, // Level 7
        17, // Level 8
        16, // Level 9
        16, // Level 10
        15, // Level 11
        15, // Level 12
        14, // Level 13
        14, // Level 14
        13, // Level 15
        13, // Level 16
        12, // Level 17
        12, // Level 18
        11, // Level 19
        11  // Level 20
    };

    static constexpr std::array<int, 21> fighter_thac0 = {
        0,  // Level 0 (unused)
        20, // Level 1
        19, // Level 2
        18, // Level 3
        17, // Level 4
        16, // Level 5
        15, // Level 6
        14, // Level 7
        13, // Level 8
        12, // Level 9
        11, // Level 10
        10, // Level 11
        9,  // Level 12
        8,  // Level 13
        7,  // Level 14
        6,  // Level 15
        5,  // Level 16
        4,  // Level 17
        3,  // Level 18
        2,  // Level 19
        1   // Level 20
    };

    static constexpr std::array<int, 21> wizard_thac0 = {
        0,  // Level 0 (unused)
        20, // Level 1
        20, // Level 2
        20, // Level 3
        19, // Level 4
        19, // Level 5
        19, // Level 6
        18, // Level 7
        18, // Level 8
        18, // Level 9
        17, // Level 10
        17, // Level 11
        17, // Level 12
        16, // Level 13
        16, // Level 14
        16, // Level 15
        15, // Level 16
        15, // Level 17
        15, // Level 18
        14, // Level 19
        14  // Level 20
    };

public:
    constexpr int get_fighter(int level) const noexcept
    {
        return (level >= 1 && level <= 20) ? fighter_thac0[level] : 20;
    }

    constexpr int get_rogue(int level) const noexcept
    {
        return (level >= 1 && level <= 20) ? rogue_thac0[level] : 20;
    }

    constexpr int get_cleric(int level) const noexcept
    {
        return (level >= 1 && level <= 20) ? cleric_thac0[level] : 20;
    }

    constexpr int get_wizard(int level) const noexcept
    {
        return (level >= 1 && level <= 20) ? wizard_thac0[level] : 20;
    }
};
