#include "Items/Armor.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "E:/repo/C++RogueLike/src/Items/Armor.h"
#include "E:/repo/C++RogueLike/src/Game.h"
#include "E:/repo/C++RogueLike/src/Colors/Colors.h"
#include "E:/repo/C++RogueLike/src/ActorTypes/Player.h"

// Mock classes to simulate dependencies
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

Game game; // Global instance for testing purposes

void test_ArmorUse() {
    Player player;
    Item item;
    Armor::apply_stat_effects(player, item);
    assert(item.has_state(ActorState::IS_EQUIPPED) && "Item should be equipped after applying effects.");

    // Test use function for player
    bool result = Armor::use(item, player);
    assert(result && "Use function should return true when successful.");

    // Check messages based on equip/unequip status
    std::string message;
    if (player.is_item_equipped(item.uniqueId)) {
        message = "You remove the " + item.actorData.name + ".";
    } else {
        message = "You put on the " + item.actorData.name + ".";
    }
    assert(game.message(WHITE_BLACK_PAIR, message, true) && "Message should be displayed.");
}

void test_LeatherArmor() {
    LeatherArmor leather;
    assert(leather.armorClass == -2 && "Leather armor should have AC of -2.");
}

void test_ChainMail() {
    ChainMail chainmail;
    assert(chainmail.armorClass == -4 && "Chain mail should have AC of -4.");
}

void test_PlateMail() {
    PlateMail platemail;
    assert(platemail.armorClass == -6 && "Plate mail should have AC of -6.");
}

int main() {
    // Run tests
    std::cout << "Running Armor use function test..." << std::endl;
    test_ArmorUse();
    std::cout << "Passed." << std::endl;

    std::cout << "Running LeatherArmor test..." << std::endl;
    test_LeatherArmor();
    std::cout << "Passed." << std::endl;

    std::cout << "Running ChainMail test..." << std::endl;
    test_ChainMail();
    std::cout << "Passed." << std::endl;

    std::cout << "Running PlateMail test..." << std::endl;
    test_PlateMail();
    std::cout << "Passed." << std::endl;

    return 0;
}