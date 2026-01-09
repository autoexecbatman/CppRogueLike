# ItemFactory Refactoring: Detailed Code Transformations

## Overview
This document shows exact before/after code transformations for all 37 `game.` references that need to be converted to GameContext dependency injection.

---

## 1. HEADER FILE TRANSFORMATIONS

### File: `src/Factories/ItemFactory.h`

#### Transformation 1.1: Add Forward Declaration
```cpp
// BEFORE (lines 1-9)
#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// AFTER (lines 1-11)
#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declaration for dependency injection
struct GameContext;
```

#### Transformation 1.2: Update Constructor and Public Methods
```cpp
// BEFORE (lines 32-38)
public:
    ItemFactory();

    void add_item_type(const ItemType& itemType);
    void generate_treasure(Vector2D position, int dungeonLevel, int quality);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);
    void spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category);
    void spawn_random_item(Vector2D position, int dungeonLevel);

// AFTER (lines 32-41)
public:
    ItemFactory();
    void init(GameContext& ctx);  // NEW: Deferred initialization for context

    void add_item_type(const ItemType& itemType);
    void generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);
    void spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category);
    void spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel);
```

#### Transformation 1.3: Add Member Variable for Context
```cpp
// BEFORE (lines 39-43)
private:
    std::vector<ItemType> itemTypes;

    // Map of item categories to filter items by type
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;

// AFTER (lines 39-45)
private:
    GameContext* ctx_ptr = nullptr;  // NEW: Store context for lambda access

    std::vector<ItemType> itemTypes;

    // Map of item categories to filter items by type
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;
```

---

## 2. CONSTRUCTOR IMPLEMENTATION TRANSFORMATIONS

### File: `src/Factories/ItemFactory.cpp`

#### Transformation 2.1: Constructor Declaration
```cpp
// BEFORE (line 13)
ItemFactory::ItemFactory()
{

// AFTER
ItemFactory::ItemFactory()
    : ctx_ptr(nullptr)
{
```

#### Transformation 2.2: Add init() Method
```cpp
// NEW METHOD (add after constructor)
void ItemFactory::init(GameContext& ctx)
{
    ctx_ptr = &ctx;
}
```

#### Transformation 2.3: Health Potion Lambda (Lines 18-23)
```cpp
// BEFORE
add_item_type({
    "Health Potion", 50, 1, 0, 0.2f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_health_potion(pos));
    }
});

// AFTER
add_item_type({
    "Health Potion", 50, 1, 0, 0.2f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_health_potion(pos));
    }
});
```

**Pattern**: Replace `game.inventory_data` with `ctx_ptr->inventory_data` in ALL lambdas (23 total)

#### Transformation 2.4: Scroll of Lightning Lambda (Lines 26-31)
```cpp
// BEFORE
add_item_type({
    "Scroll of Lightning Bolt", 20, 2, 0, 0.2f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_scroll_lightning(pos));
    }
});

// AFTER
add_item_type({
    "Scroll of Lightning Bolt", 20, 2, 0, 0.2f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_scroll_lightning(pos));
    }
});
```

#### Transformation 2.5: Scroll of Fireball Lambda (Lines 33-38)
```cpp
// BEFORE
add_item_type({
    "Scroll of Fireball", 15, 3, 0, 0.3f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_scroll_fireball(pos));
    }
});

// AFTER
add_item_type({
    "Scroll of Fireball", 15, 3, 0, 0.3f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_scroll_fireball(pos));
    }
});
```

#### Transformation 2.6: Scroll of Confusion Lambda (Lines 40-45)
```cpp
// BEFORE
add_item_type({
    "Scroll of Confusion", 15, 2, 0, 0.2f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_scroll_confusion(pos));
    }
});

// AFTER
add_item_type({
    "Scroll of Confusion", 15, 2, 0, 0.2f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_scroll_confusion(pos));
    }
});
```

#### Transformation 2.7: Dagger Lambda with Enhancement (Lines 48-53)
```cpp
// BEFORE
add_item_type({
    "Dagger", 3, 1, 3, -0.5f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_enhanced_dagger(pos,
            ItemCreator::determine_enhancement_level(game.level_manager.get_dungeon_level())));
    }
});

// AFTER
add_item_type({
    "Dagger", 3, 1, 3, -0.5f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_enhanced_dagger(pos,
            ItemCreator::determine_enhancement_level(ctx_ptr->level_manager->get_dungeon_level())));
    }
});
```

**Pattern**:
- Change `game.inventory_data` → `ctx_ptr->inventory_data`
- Change `game.level_manager.get_dungeon_level()` → `ctx_ptr->level_manager->get_dungeon_level()`

#### Transformation 2.8: Short Sword Lambda (Lines 55-60)
```cpp
// BEFORE
add_item_type({
    "Short Sword", 5, 1, 4, -0.4f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_enhanced_short_sword(pos,
            ItemCreator::determine_enhancement_level(game.level_manager.get_dungeon_level())));
    }
});

// AFTER
add_item_type({
    "Short Sword", 5, 1, 4, -0.4f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_enhanced_short_sword(pos,
            ItemCreator::determine_enhancement_level(ctx_ptr->level_manager->get_dungeon_level())));
    }
});
```

#### Transformation 2.9: Long Sword Lambda (Lines 62-67)
```cpp
// BEFORE
add_item_type({
    "Long Sword", 6, 1, 0, -0.2f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_enhanced_long_sword(pos,
            ItemCreator::determine_enhancement_level(game.level_manager.get_dungeon_level())));
    }
});

// AFTER
add_item_type({
    "Long Sword", 6, 1, 0, -0.2f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_enhanced_long_sword(pos,
            ItemCreator::determine_enhancement_level(ctx_ptr->level_manager->get_dungeon_level())));
    }
});
```

#### Transformation 2.10: Staff Lambda (Lines 69-74)
```cpp
// BEFORE
add_item_type({
    "Staff", 8, 2, 0, 0.1f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_enhanced_staff(pos,
            ItemCreator::determine_enhancement_level(game.level_manager.get_dungeon_level())));
    }
});

// AFTER
add_item_type({
    "Staff", 8, 2, 0, 0.1f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_enhanced_staff(pos,
            ItemCreator::determine_enhancement_level(ctx_ptr->level_manager->get_dungeon_level())));
    }
});
```

#### Transformation 2.11: Longbow Lambda (Lines 76-81)
```cpp
// BEFORE
add_item_type({
    "Longbow", 12, 3, 0, 0.3f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_enhanced_longbow(pos,
            ItemCreator::determine_enhancement_level(game.level_manager.get_dungeon_level())));
    }
});

// AFTER
add_item_type({
    "Longbow", 12, 3, 0, 0.3f,
    [this](Vector2D pos) {
        add_item(ctx_ptr->inventory_data, ItemCreator::create_enhanced_longbow(pos,
            ItemCreator::determine_enhancement_level(ctx_ptr->level_manager->get_dungeon_level())));
    }
});
```

#### Transformation 2.12: Gold Lambda (Lines 84-87)
```cpp
// BEFORE
add_item_type({
    "Gold", 25, 1, 0, 0.1f,
    [](Vector2D pos) { add_item(game.inventory_data, ItemCreator::create_gold_pile(pos)); }
});

// AFTER
add_item_type({
    "Gold", 25, 1, 0, 0.1f,
    [this](Vector2D pos) { add_item(ctx_ptr->inventory_data, ItemCreator::create_gold_pile(pos)); }
});
```

#### Transformation 2.13: Ration Lambda (Lines 90-93)
```cpp
// BEFORE
add_item_type({
    "Ration", 25, 1, 0, 0.1f,
    [](Vector2D pos) { add_item(game.inventory_data, ItemCreator::create_ration(pos)); }
});

// AFTER
add_item_type({
    "Ration", 25, 1, 0, 0.1f,
    [this](Vector2D pos) { add_item(ctx_ptr->inventory_data, ItemCreator::create_ration(pos)); }
});
```

#### Transformation 2.14-2.18: Food Items (Fruit, Bread, Meat, Amulet, Armor)
```cpp
// BEFORE (Lines 95-135) - ALL FOOD/ARMOR LAMBDAS
[](Vector2D pos) { ... game.inventory_data ... }

// AFTER
[this](Vector2D pos) { ... ctx_ptr->inventory_data ... }
```

---

## 3. GENERATE_TREASURE METHOD TRANSFORMATIONS

### File: `src/Factories/ItemFactory.cpp` (Lines 177-275)

#### Transformation 3.1: Method Signature
```cpp
// BEFORE (line 177)
void ItemFactory::generate_treasure(Vector2D position, int dungeonLevel, int quality)
{

// AFTER
void ItemFactory::generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality)
{
```

#### Transformation 3.2: Dice Rolls for Item Count (Lines 188-200)
```cpp
// BEFORE (lines 186-205)
case 1:
{
    itemCount = game.d.roll(1, 2);
    break;
}

case 2:
{
    itemCount = game.d.roll(2, 3);
    break;
}

case 3:
{
    itemCount = game.d.roll(3, 5);
    break;
}

// AFTER
case 1:
{
    itemCount = ctx.dice->roll(1, 2);
    break;
}

case 2:
{
    itemCount = ctx.dice->roll(2, 3);
    break;
}

case 3:
{
    itemCount = ctx.dice->roll(3, 5);
    break;
}
```

**Pattern**: Replace `game.d.roll()` with `ctx.dice->roll()`

#### Transformation 3.3: Gold Amount Roll (Line 213)
```cpp
// BEFORE
int goldAmount = game.d.roll(goldMin, goldMax);

// AFTER
int goldAmount = ctx.dice->roll(goldMin, goldMax);
```

#### Transformation 3.4: Create and Add Gold Pile (Lines 216-218)
```cpp
// BEFORE
auto goldPile = std::make_unique<Item>(position, ActorData{ '$', "gold pile", YELLOW_BLACK_PAIR });
goldPile->pickable = std::make_unique<Gold>(goldAmount);
add_item(game.inventory_data, std::move(goldPile));

// AFTER
auto goldPile = std::make_unique<Item>(position, ActorData{ '$', "gold pile", YELLOW_BLACK_PAIR });
goldPile->pickable = std::make_unique<Gold>(goldAmount);
add_item(ctx.inventory_data, std::move(goldPile));
```

#### Transformation 3.5: Item Position Offset (Lines 225-226)
```cpp
// BEFORE
Vector2D itemPos = position;
itemPos.x += game.d.roll(-1, 1);
itemPos.y += game.d.roll(-1, 1);

// AFTER
Vector2D itemPos = position;
itemPos.x += ctx.dice->roll(-1, 1);
itemPos.y += ctx.dice->roll(-1, 1);
```

#### Transformation 3.6: Walkability Check (Line 229)
```cpp
// BEFORE
if (!game.map.can_walk(itemPos))

// AFTER
if (!ctx.map->can_walk(itemPos))
```

#### Transformation 3.7: Quality Roll (Line 235)
```cpp
// BEFORE
int roll = game.d.d100();

// AFTER
int roll = ctx.dice->d100();
```

#### Transformation 3.8: Artifact Quality Check (Line 240)
```cpp
// BEFORE
if (effectiveLevel >= 8 && game.d.d100() <= 10)

// AFTER
if (effectiveLevel >= 8 && ctx.dice->d100() <= 10)
```

#### Transformation 3.9: Spawn Item Calls (Lines 243-269)
```cpp
// BEFORE
spawn_item_of_category(itemPos, effectiveLevel, "artifact");
spawn_item_of_category(itemPos, effectiveLevel + 2, "weapon");
spawn_item_of_category(itemPos, effectiveLevel, "weapon");
spawn_item_of_category(itemPos, effectiveLevel, "scroll");
spawn_item_of_category(itemPos, effectiveLevel, "potion");
spawn_item_of_category(itemPos, effectiveLevel, "food");

// AFTER
spawn_item_of_category(ctx, itemPos, effectiveLevel, "artifact");
spawn_item_of_category(ctx, itemPos, effectiveLevel + 2, "weapon");
spawn_item_of_category(ctx, itemPos, effectiveLevel, "weapon");
spawn_item_of_category(ctx, itemPos, effectiveLevel, "scroll");
spawn_item_of_category(ctx, itemPos, effectiveLevel, "potion");
spawn_item_of_category(ctx, itemPos, effectiveLevel, "food");
```

**Pattern**: All calls to spawn_item_of_category now pass ctx as first parameter

#### Transformation 3.10: Log Treasure Generation (Line 273)
```cpp
// BEFORE
game.log("Generated treasure of quality " + std::to_string(quality) +
    " with " + std::to_string(itemCount + 1) + " items including gold");

// AFTER
ctx.message_system->log("Generated treasure of quality " + std::to_string(quality) +
    " with " + std::to_string(itemCount + 1) + " items including gold");
```

**Pattern**: Replace `game.log()` with `ctx.message_system->log()`

---

## 4. SPAWN_ITEM_OF_CATEGORY METHOD TRANSFORMATIONS

### File: `src/Factories/ItemFactory.cpp` (Lines 307-351)

#### Transformation 4.1: Method Signature
```cpp
// BEFORE (line 307)
void ItemFactory::spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category)
{

// AFTER
void ItemFactory::spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category)
{
```

#### Transformation 4.2: Category Not Found Log (Line 313)
```cpp
// BEFORE
game.log("No items found in category: " + category);

// AFTER
ctx.message_system->log("No items found in category: " + category);
```

#### Transformation 4.3: Random Roll for Item (Line 338)
```cpp
// BEFORE
int roll = game.d.roll(1, totalWeight);

// AFTER
int roll = ctx.dice->roll(1, totalWeight);
```

---

## 5. SPAWN_RANDOM_ITEM METHOD TRANSFORMATIONS

### File: `src/Factories/ItemFactory.cpp` (Lines 354-388)

#### Transformation 5.1: Method Signature
```cpp
// BEFORE (line 354)
void ItemFactory::spawn_random_item(Vector2D position, int dungeonLevel)
{

// AFTER
void ItemFactory::spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel)
{
```

#### Transformation 5.2: No Valid Items Log (Line 370)
```cpp
// BEFORE
game.log("No valid items for this dungeon level!");

// AFTER
ctx.message_system->log("No valid items for this dungeon level!");
```

#### Transformation 5.3: Random Roll for Item (Line 375)
```cpp
// BEFORE
int roll = game.d.roll(1, totalWeight);

// AFTER
int roll = ctx.dice->roll(1, totalWeight);
```

---

## 6. CALL SITE TRANSFORMATIONS

### Location: `src/Map/Map.cpp`

#### Transformation 6.1: ItemFactory Initialization (Lines 222-224)
```cpp
// BEFORE (Map::Map constructor)
monsterFactory(std::make_unique<MonsterFactory>()),
itemFactory(std::make_unique<ItemFactory>())
{
}

// AFTER (No change to constructor - Map already has default constructor)
// Add init call in Map::init() method instead (see below)
```

#### Transformation 6.2: Add init() Call in Map::init()
```cpp
// ADD to Map::init() after basic initialization:
if (itemFactory)
{
    itemFactory->init(ctx);
}
```

#### Transformation 6.3: spawn_items Method Call (Line 581)
```cpp
// BEFORE (in Map::spawn_items method)
if (ctx.level_manager)
{
    itemFactory->spawn_random_item(pos, ctx.level_manager->get_dungeon_level());
}

// AFTER
if (ctx.level_manager)
{
    itemFactory->spawn_random_item(ctx, pos, ctx.level_manager->get_dungeon_level());
}
```

#### Transformation 6.4: generate_treasure Method Call (Line 1297)
```cpp
// BEFORE (in Map bsp method)
itemFactory->generate_treasure(center, ctx.level_manager->get_dungeon_level(), quality);

// AFTER
itemFactory->generate_treasure(ctx, center, ctx.level_manager->get_dungeon_level(), quality);
```

---

## 7. LOGGING: SPAWN_ITEM_OF_CATEGORY MISSING LOG

#### Transformation 7.1: No Valid Items in Category (Line 333)
```cpp
// BEFORE
game.log("No valid items in category " + category + " for this dungeon level!");

// AFTER
ctx.message_system->log("No valid items in category " + category + " for this dungeon level!");
```

---

## 8. SUMMARY OF TRANSFORMATION PATTERNS

### Pattern 1: Simple Inventory Access
```cpp
game.inventory_data          → ctx_ptr->inventory_data          (in lambdas)
game.inventory_data          → ctx.inventory_data               (in methods)
```

### Pattern 2: Dice Rolls
```cpp
game.d.roll(min, max)        → ctx.dice->roll(min, max)
game.d.d100()                → ctx.dice->d100()
```

### Pattern 3: Level Manager Access
```cpp
game.level_manager.get_dungeon_level()  → ctx.level_manager->get_dungeon_level()
```

### Pattern 4: Map Access
```cpp
game.map.can_walk(pos)       → ctx.map->can_walk(pos)
```

### Pattern 5: Logging
```cpp
game.log(msg)                → ctx.message_system->log(msg)
```

### Pattern 6: Method Signature Updates
```cpp
void method(Vector2D, ...)   → void method(GameContext& ctx, Vector2D, ...)
```

### Pattern 7: Lambda Captures
```cpp
[](Vector2D pos)             → [this](Vector2D pos)
```

---

## 9. FILES MODIFIED SUMMARY

| File | Changes | References |
|------|---------|-----------|
| ItemFactory.h | +1 forward decl, +1 member var, +1 method sig, 3 method sigs | 5 |
| ItemFactory.cpp | +1 init() method, constructor init, 23 lambdas, 3 methods | 37 |
| Map.cpp | +1 init call, 2 call sites | 2 |
| **TOTAL** | | **44** |

---

## 10. VERIFICATION CHECKLIST

Before and after refactoring, verify:

- [ ] No `extern Game game;` declaration in ItemFactory.cpp
- [ ] No `game.` references in ItemFactory.cpp
- [ ] All lambdas use `[this]` capture and access `ctx_ptr`
- [ ] All methods accept `GameContext& ctx` parameter
- [ ] init() is called in Map::init() with context
- [ ] Both Map call sites pass ctx parameter
- [ ] Tests compile and pass with new signatures
- [ ] No crashes due to null ctx_ptr (add assertions if needed)

---

## 11. OPTIONAL SAFEGUARDS

### Add Assertion to init()
```cpp
void ItemFactory::init(GameContext& ctx)
{
    ctx_ptr = &ctx;
    // Optional: Verify all required pointers are non-null
    assert(ctx_ptr->inventory_data != nullptr);
    assert(ctx_ptr->dice != nullptr);
    assert(ctx_ptr->message_system != nullptr);
}
```

### Add Assertion to Public Methods
```cpp
void ItemFactory::spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel)
{
    // Optional: Debug check that context is initialized
    assert(ctx_ptr != nullptr || ctx_ptr == &ctx);
    // ... rest of method
}
```

This provides runtime safety without significant performance overhead.
