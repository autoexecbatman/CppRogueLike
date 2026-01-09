#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file
#include "E:/repo/C++RogueLike/src/Items/Mace.cpp"

void test_MaceInitialization() {
    Mace mace;
    assert(mace.get_type() == Pickable::PickableType::MACE && "Mace type should be MACE");
}

void test_MaceUse() {
    // Mock classes for testing
    class Item {};
    class Creature {};
    Game game;  // Assuming Game is a mock or existing class

    Item owner;
    std::string ownerName = "TestOwner";
    owner.actorData.name = ownerName;
    Creature wearer;

    Mace mace;
    bool result = mace.use(owner, wearer);
    assert(result == true && "Mace use should return true");
}

void test_MaceSaveAndLoad() {
    json j;
    Mace mace;
    mace.save(j);

    // Check if the type is correctly saved and loaded
    int expectedType = static_cast<int>(Pickable::PickableType::MACE);
    assert(j["type"] == expectedType && "Mace type should be correctly saved and loaded");
}

int main() {
    try {
        test_MaceInitialization();
        test_MaceUse();
        test_MaceSaveAndLoad();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}