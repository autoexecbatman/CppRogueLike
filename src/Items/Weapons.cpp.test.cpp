#include "Items/Weapons.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "Weapons.h"

void test_get_damage_roll() {
    // Create an instance of Weapons to run tests on
    Weapons weapon;

    // Test default damage roll (should not be two-handed)
    std::string defaultRoll = weapon.get_damage_roll(false);
    assert(defaultRoll == "1d8" && "Default damage roll should be '1d8'");

    // Test two-handed damage roll
    std::string twoHandedRoll = weapon.get_damage_roll(true);
    assert(twoHandedRoll == "" && "Two-handed damage roll should be empty if not set");

    // Set a two-handed damage roll and test again
    weapon.damageRollTwoHanded = "2d10";
    twoHandedRoll = weapon.get_damage_roll(true);
    assert(twoHandedRoll == "2d10" && "Two-handed damage roll should return '2d10' if set");
}

int main() {
    try {
        test_get_damage_roll();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1; // Return non-zero to indicate failure
    }

    return 0; // Return zero to indicate success
}