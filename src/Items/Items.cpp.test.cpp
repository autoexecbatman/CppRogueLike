#include "Items/Items.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "Items/Items.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../ActorTypes/Teleporter.h"
#include "../ActorTypes/Gold.h"
#include "Amulet.h"
#include "Armor.h"

void test_HealthPotion() {
    Vector2D pos(0, 0);
    HealthPotion potion(pos);
    assert(potion.getSymbol() == '!');
    assert(potion.getName() == "health potion");
    assert(potion.getValue() == 50);
    assert(dynamic_cast<Healer*>(potion.getPickable().get()) != nullptr);
}

void test_ScrollOfLightningBolt() {
    Vector2D pos(0, 0);
    ScrollOfLightningBolt scroll(pos);
    assert(scroll.getSymbol() == '#');
    assert(scroll.getName() == "scroll of lightning bolt");
    assert(scroll.getValue() == 150);
    assert(dynamic_cast<LightningBolt*>(scroll.getPickable().get()) != nullptr);
}

void test_ScrollOfFireball() {
    Vector2D pos(0, 0);
    ScrollOfFireball scroll(pos);
    assert(scroll.getSymbol() == '#');
    assert(scroll.getName() == "scroll of fireball");
    assert(scroll.getValue() == 100);
    assert(dynamic_cast<Fireball*>(scroll.getPickable().get()) != nullptr);
}

void test_ScrollOfConfusion() {
    Vector2D pos(0, 0);
    ScrollOfConfusion scroll(pos);
    assert(scroll.getSymbol() == '#');
    assert(scroll.getName() == "scroll of confusion");
    assert(scroll.getValue() == 120);
    assert(dynamic_cast<Confuser*>(scroll.getPickable().get()) != nullptr);
}

void test_ScrollOfTeleportation() {
    Vector2D pos(0, 0);
    ScrollOfTeleportation scroll(pos);
    assert(scroll.getSymbol() == '#');
    assert(scroll.getName() == "scroll of teleportation");
    assert(scroll.getValue() == 200);
    assert(dynamic_cast<Teleporter*>(scroll.getPickable().get()) != nullptr);
}

void test_GoldPile() {
    Vector2D pos(0, 0);
    Game game; // Assuming a simple instantiation for testing purposes
    GoldPile goldPile(pos);
    assert(goldPile.getSymbol() == '$');
    assert(goldPile.getName() == "gold pile");
    assert(goldPile.getValue() >= 5 && goldPile.getValue() <= 20 + game.level_manager.get_dungeon_level() * 3);
    assert(dynamic_cast<Gold*>(goldPile.getPickable().get()) != nullptr);
}

void test_AmuletOfYendor() {
    Vector2D pos(0, 0);
    AmuletOfYendor amulet(pos);
    assert(amulet.getSymbol() == '*');
    assert(amulet.getName() == "Amulet of Yendor");
    assert(amulet.getValue() == 1000);
    assert(dynamic_cast<Amulet*>(amulet.getPickable().get()) != nullptr);
}


int main() {
    try {
        test_HealthPotion();
        std::cout << "HealthPotion test passed." << std::endl;
        test_ScrollOfLightningBolt();
        std::cout << "ScrollOfLightningBolt test passed." << std::endl;
        test_ScrollOfFireball();
        std::cout << "ScrollOfFireball test passed." << std::endl;
        test_ScrollOfConfusion();
        std::cout << "ScrollOfConfusion test passed." << std::endl;
        test_ScrollOfTeleportation();
        std::cout << "ScrollOfTeleportation test passed." << std::endl;
        test_GoldPile();
        std::cout << "GoldPile test passed." << std::endl;
        test_AmuletOfYendor();
        std::cout << "AmuletOfYendor test passed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
    }

    return 0;
}