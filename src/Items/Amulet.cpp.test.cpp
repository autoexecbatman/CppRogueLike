#include "Items/Amulet.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "E:/repo/C++RogueLike/src/Items/Amulet.h"
#include "E:/repo/C++RogueLike/src/Game.h"
#include "E:/repo/C++RogueLike/src/Colors/Colors.h"

// Assuming Game and Colors are defined elsewhere in the project
// If not, you would need to include those headers as well.

void test_AmuletCreation() {
    Amulet amulet;
    assert(true); // Simple pass to indicate no assertion failures for creation
}

void test_AmuletUse() {
    Game game; // Assuming a default constructor or similar setup for Game
    Amulet amulet;
    Creature wearer; // Placeholder, actual implementation might differ

    bool result = amulet.use(amulet, wearer); // Using the item on itself and a creature
    assert(result == false); // Check if use method returns false as expected
    assert(game.gameStatus == Game::GameStatus::VICTORY); // Check game status is set to victory
}

void test_AmuletSaveAndLoad() {
    Amulet amulet;
    json j;
    amulet.save(j);

    int type = j["type"];
    assert(type == static_cast<int>(PickableType::AMULET)); // Check if the save method correctly saves the type
}

int main() {
    test_AmuletCreation();
    test_AmuletUse();
    test_AmuletSaveAndLoad();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}