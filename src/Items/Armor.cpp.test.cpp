#include "Items/Armor.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

#include "E:/repo/C++RogueLike/src/Items/Armor.h"
#include "E:/repo/C++RogueLike/src/Game.h"
#include "E:/repo/C++RogueLike/src/Colors/Colors.h"
#include "E:/repo/C++RogueLike/src/ActorTypes/Player.h"

class Creature {
public:
    class Destructible {
    public:
        void update_armor_class(Creature& creature) {}
    };
    Destructible* destructible;
};

class Item {
public:
    std::string name = "Test Armor";
    int uniqueId = 1;
    class ActorData {
    public:
        std::string name = "Test Armor";
    };
    ActorData actorData;
    bool has_state(ActorState state) { return false; }
    void add_state(ActorState state) {}
    void remove_state(ActorState state) {}
};

class Player : public Creature {
public:
    bool is_item_equipped(int id) { return false; }
    bool toggle_armor(int id) { return true; }
};

Game game;

void test_Armor_ac_bonus()
{
    Armor leather(-2);
    assert(leather.get_ac_bonus() == -2 && "Leather armor AC bonus should be -2.");

    Armor chain(-5);
    assert(chain.get_ac_bonus() == -5 && "Chain mail AC bonus should be -5.");

    Armor plate(-7);
    assert(plate.get_ac_bonus() == -7 && "Plate mail AC bonus should be -7.");

    Armor fullPlate(-9);
    assert(fullPlate.get_ac_bonus() == -9 && "Full plate AC bonus should be -9.");
}

int main()
{
    std::cout << "Running Armor AC bonus test..." << std::endl;
    test_Armor_ac_bonus();
    std::cout << "Passed." << std::endl;

    return 0;
}
