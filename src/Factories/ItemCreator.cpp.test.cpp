#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "Factories/ItemCreator.cpp"
#include "Renderer/TileId.h"

void test_create_health_potion() {
    ItemCreator creator;
    Vector2D pos(0, 0);
    auto item = creator.create_health_potion(pos);
    
    assert(item->actorData.ch == TILE_POTION);
    assert(item->actorData.name == "health potion");
    assert(item->pickable != nullptr);
    assert(item->value == 50);

    std::cout << "test_create_health_potion passed\n";
}

void test_create_scroll_lightning() {
    ItemCreator creator;
    Vector2D pos(0, 0);
    auto item = creator.create_scroll_lightning(pos);

    assert(item->actorData.ch == TILE_SCROLL);
    assert(item->actorData.name == "scroll of lightning bolt");
    assert(item->pickable != nullptr);
    assert(item->value == 150);

    std::cout << "test_create_scroll_lightning passed\n";
}

void test_create_random_potion() {
    ItemCreator creator;
    Vector2D pos(0, 0);
    auto item = creator.create_random_potion(pos, 1);

    assert(item->actorData.ch == TILE_POTION);
    assert(item->actorData.name == "health potion");
    assert(item->pickable != nullptr);
    assert(item->value == 50);
    
    std::cout << "test_create_random_potion passed\n";
}

void test_calculate_enhancement_chance() {
    ItemCreator creator;
    int chance = creator.calculate_enhancement_chance(1);
    assert(chance == 5);
    
    chance = creator.calculate_enhancement_chance(2);
    assert(chance == 8);
    
    chance = creator.calculate_enhancement_chance(3);
    assert(chance == 11);
    
    chance = creator.calculate_enhancement_chance(10);
    assert(chance == 35);
    
    std::cout << "test_calculate_enhancement_chance passed\n";
}

void test_determine_enhancement_level() {
    ItemCreator creator;
    int level = creator.determine_enhancement_level(1);
    assert(level >= 0 && level <= 3);
    
    level = creator.determine_enhancement_level(5);
    assert(level >= 0 && level <= 3);
    
    std::cout << "test_determine_enhancement_level passed\n";
}

void test_create_enhanced_dagger() {
    ItemCreator creator;
    Vector2D pos(0, 0);
    auto item = creator.create_enhanced_dagger(pos, 1);
    
    assert(item->actorData.color == WHITE_GREEN_PAIR);
    assert(item->value > 50 && item->value < 75); // Enhanced value should be higher than base dagger but less than enhanced short sword
    
    std::cout << "test_create_enhanced_dagger passed\n";
}

void test_create_leather_armor() {
    ItemCreator creator;
    Vector2D pos(0, 0);
    auto item = creator.create_leather_armor(pos);
    
    assert(item->actorData.ch == TILE_ARMOR);
    assert(item->actorData.name == "leather armor");
    assert(item->pickable != nullptr);
    assert(item->value == 5);
    
    std::cout << "test_create_leather_armor passed\n";
}

void run_all_tests() {
    test_create_health_potion();
    test_create_scroll_lightning();
    test_create_random_potion();
    test_calculate_enhancement_chance();
    test_determine_enhancement_level();
    test_create_enhanced_dagger();
    test_create_leather_armor();
}

int main() {
    run_all_tests();
    std::cout << "All tests passed.\n";
    return 0;
}