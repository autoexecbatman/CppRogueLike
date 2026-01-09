# ItemFactory Refactoring: Quick Reference Guide

## Quick Lookup: game. → ctx. Replacements

### Global Reference (Remove from ItemFactory.cpp)
```diff
- extern Game game;
```

### Pattern 1: Inventory Access (24 refs)

**In lambdas** (constructor, 23 refs):
```diff
- [](Vector2D pos) { add_item(game.inventory_data, ...) }
+ [this](Vector2D pos) { add_item(ctx_ptr->inventory_data, ...) }
```

**In method bodies** (1 ref in generate_treasure):
```diff
- add_item(game.inventory_data, std::move(goldPile));
+ add_item(ctx.inventory_data, std::move(goldPile));
```

### Pattern 2: Dice Rolls (13 refs)

**Single roll**:
```diff
- game.d.roll(1, 2)
+ ctx.dice->roll(1, 2)
```

**D100 roll**:
```diff
- game.d.d100()
+ ctx.dice->d100()
```

**Examples**:
- Line 188: `game.d.roll(1, 2)` → `ctx.dice->roll(1, 2)`
- Line 194: `game.d.roll(2, 3)` → `ctx.dice->roll(2, 3)`
- Line 200: `game.d.roll(3, 5)` → `ctx.dice->roll(3, 5)`
- Line 213: `game.d.roll(goldMin, goldMax)` → `ctx.dice->roll(goldMin, goldMax)`
- Line 225: `game.d.roll(-1, 1)` → `ctx.dice->roll(-1, 1)` (2 refs)
- Line 235: `game.d.d100()` → `ctx.dice->d100()`
- Line 240: `game.d.d100()` → `ctx.dice->d100()`
- Line 338: `game.d.roll(1, totalWeight)` → `ctx.dice->roll(1, totalWeight)`
- Line 375: `game.d.roll(1, totalWeight)` → `ctx.dice->roll(1, totalWeight)`

**Total Dice Refs**: 13

### Pattern 3: Level Manager (5 refs)

**Access current level** (all in lambdas):
```diff
- game.level_manager.get_dungeon_level()
+ ctx_ptr->level_manager->get_dungeon_level()
```

**Locations**:
- Line 51 (Dagger): `game.level_manager.get_dungeon_level()`
- Line 58 (Short Sword): `game.level_manager.get_dungeon_level()`
- Line 65 (Long Sword): `game.level_manager.get_dungeon_level()`
- Line 72 (Staff): `game.level_manager.get_dungeon_level()`
- Line 79 (Longbow): `game.level_manager.get_dungeon_level()`

**Total Level Manager Refs**: 5

### Pattern 4: Logging (4 refs)

**Message system calls**:
```diff
- game.log("message");
+ ctx.message_system->log("message");
```

**Locations**:
- Line 273 (generate_treasure): `game.log("Generated treasure...")`
- Line 313 (spawn_item_of_category): `game.log("No items found...")`
- Line 333 (spawn_item_of_category): `game.log("No valid items...")`
- Line 370 (spawn_random_item): `game.log("No valid items...")`

**Total Logging Refs**: 4

### Pattern 5: Map Access (1 ref)

**Walkability check**:
```diff
- game.map.can_walk(itemPos)
+ ctx.map->can_walk(itemPos)
```

**Location**:
- Line 229 (generate_treasure): `game.map.can_walk(itemPos)`

**Total Map Refs**: 1

---

## Method Signature Changes

### generate_treasure
```diff
- void ItemFactory::generate_treasure(Vector2D position, int dungeonLevel, int quality)
+ void ItemFactory::generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality)
```

### spawn_item_of_category
```diff
- void ItemFactory::spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category)
+ void ItemFactory::spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category)
```

### spawn_random_item
```diff
- void ItemFactory::spawn_random_item(Vector2D position, int dungeonLevel)
+ void ItemFactory::spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel)
```

---

## Call Site Updates (Map.cpp)

### Location 1: Map::spawn_items() - Line 581
```diff
- itemFactory->spawn_random_item(pos, ctx.level_manager->get_dungeon_level());
+ itemFactory->spawn_random_item(ctx, pos, ctx.level_manager->get_dungeon_level());
```

### Location 2: Map bsp() - Line 1297
```diff
- itemFactory->generate_treasure(center, ctx.level_manager->get_dungeon_level(), quality);
+ itemFactory->generate_treasure(ctx, center, ctx.level_manager->get_dungeon_level(), quality);
```

### Location 3: Map::init() - NEW ADDITION
Add this call after itemFactory creation:
```cpp
if (itemFactory)
{
    itemFactory->init(ctx);
}
```

---

## Reference Count by Location

### ItemFactory.cpp

**Constructor (lines 18-135)**: 23 lambda refs
- Inventory: 23 refs
- Level Manager: 5 refs (within lambdas)
- **Subtotal**: 28 refs

**generate_treasure (lines 177-275)**: 11 refs
- Inventory: 1 ref (line 218)
- Dice: 9 refs (lines 188, 194, 200, 213, 225, 226, 235, 240)
- Map: 1 ref (line 229)
- Logging: 1 ref (line 273)
- **Subtotal**: 12 refs

**spawn_item_of_category (lines 307-351)**: 3 refs
- Dice: 1 ref (line 338)
- Logging: 2 refs (lines 313, 333)
- **Subtotal**: 3 refs

**spawn_random_item (lines 354-388)**: 2 refs
- Dice: 1 ref (line 375)
- Logging: 1 ref (line 370)
- **Subtotal**: 2 refs

**TOTAL ITEMFACTORY.CPP**: 47 refs (28 + 12 + 3 + 2)

### Map.cpp

**Call sites**: 2 refs
- Line 581: `spawn_random_item` call
- Line 1297: `generate_treasure` call
- **Subtotal**: 2 refs

**New additions**: 1 init call
- In Map::init(): `itemFactory->init(ctx)`
- **Subtotal**: 1 addition

**TOTAL MAP.CPP**: 2 refs + 1 addition

---

## File Structure After Refactoring

### ItemFactory.h
```cpp
#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declaration for dependency injection
struct GameContext;

// ... ItemType and ItemPercentage structs unchanged ...

class ItemFactory
{
public:
    ItemFactory();
    void init(GameContext& ctx);  // NEW

    void add_item_type(const ItemType& itemType);
    void generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);
    void spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category);
    void spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel);
private:
    GameContext* ctx_ptr = nullptr;  // NEW
    std::vector<ItemType> itemTypes;
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;
    int calculate_weight(const ItemType& item, int dungeonLevel) const;
};
```

### ItemFactory.cpp - New Elements
```cpp
#include "ItemFactory.h"
#include "../Core/GameContext.h"  // NEW
// ... other includes unchanged ...

ItemFactory::ItemFactory()
    : ctx_ptr(nullptr)  // NEW
{
    // ... rest of constructor ...
}

// NEW METHOD
void ItemFactory::init(GameContext& ctx)
{
    ctx_ptr = &ctx;
}

// Updated method signatures
void ItemFactory::generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality)
void ItemFactory::spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category)
void ItemFactory::spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel)
```

---

## Lambda Transformation Template

### Before
```cpp
[](Vector2D pos) {
    // Operation using global game object
    add_item(game.inventory_data, ItemCreator::create_something(pos));
    // Possibly accessing game.level_manager
    int level = game.level_manager.get_dungeon_level();
}
```

### After
```cpp
[this](Vector2D pos) {
    // Operation using member ctx_ptr
    add_item(ctx_ptr->inventory_data, ItemCreator::create_something(pos));
    // Accessing level through ctx_ptr
    int level = ctx_ptr->level_manager->get_dungeon_level();
}
```

**Key Change**:
- Capture from empty `[]` to `[this]`
- Access via `ctx_ptr->` instead of `game.`

---

## Testing Checklist

### Compilation
- [ ] ItemFactory.h has GameContext forward declaration
- [ ] ItemFactory.cpp includes GameContext.h
- [ ] No compilation errors
- [ ] All references updated

### Runtime
- [ ] No crashes on Map construction
- [ ] itemFactory->init(ctx) called successfully
- [ ] spawn_random_item works correctly
- [ ] generate_treasure works correctly
- [ ] Items spawn in correct quantities
- [ ] Items have correct properties
- [ ] No logging errors

### Verification
- [ ] Grep for `extern Game game` in ItemFactory files - should return 0 results
- [ ] Grep for `game\.` in ItemFactory files - should return 0 results
- [ ] All 47 references replaced
- [ ] All 2 call sites updated
- [ ] All 1 init call added

---

## Common Mistakes to Avoid

### ❌ Don't do this:
```cpp
// Wrong: Forgetting to update lambda capture
[](Vector2D pos) { add_item(ctx_ptr->inventory_data, ...); }  // Error: ctx_ptr not accessible

// Wrong: Mixing game and ctx in same method
itemFactory->spawn_random_item(ctx, pos, game.level_manager.get_dungeon_level());

// Wrong: Forgetting to add init() call in Map::init()
// ... itemFactory won't have ctx_ptr set to valid pointer ...

// Wrong: Using member access on pointer
ctx.inventory_data  // Should be ctx->inventory_data in non-reference contexts
```

### ✓ Do this:
```cpp
// Right: Update lambda capture to [this]
[this](Vector2D pos) { add_item(ctx_ptr->inventory_data, ...); }

// Right: Use ctx consistently
itemFactory->spawn_random_item(ctx, pos, ctx.level_manager->get_dungeon_level());

// Right: Call init() in Map::init()
itemFactory->init(ctx);

// Right: Use proper pointer/reference syntax
ctx->inventory_data  // In pointer context (method parameter)
ctx.inventory_data   // In reference context (local variable)
```

---

## Diff Summary Statistics

| Category | Count | Details |
|----------|-------|---------|
| Inventory refs | 24 | 23 lambdas + 1 method |
| Dice refs | 13 | rolls + d100 calls |
| Level refs | 5 | dungeon level access |
| Log refs | 4 | message logging |
| Map refs | 1 | walkability check |
| **Total game. refs** | **47** | |
| Method sigs | 3 | generate, spawn_category, spawn_random |
| Call sites | 2 | Map.cpp lines 581, 1297 |
| New methods | 1 | init() in ItemFactory |
| Header changes | 2 | forward decl + member var |
| **Total changes** | **55** | |

---

## Implementation Order

1. **Header file** (ItemFactory.h)
   - Add forward declaration: `struct GameContext;`
   - Add member: `GameContext* ctx_ptr = nullptr;`
   - Update 3 public method signatures
   - Add init() method declaration

2. **Constructor** (ItemFactory.cpp)
   - Initialize ctx_ptr in initializer list
   - Update all 23 lambdas to `[this]` capture

3. **init() method** (ItemFactory.cpp)
   - Add new method implementation

4. **Method implementations** (ItemFactory.cpp)
   - Replace 12 refs in generate_treasure()
   - Replace 3 refs in spawn_item_of_category()
   - Replace 2 refs in spawn_random_item()

5. **Call sites** (Map.cpp)
   - Add init() call in Map::init()
   - Update 2 existing call sites

6. **Testing**
   - Verify compilation
   - Run unit tests
   - Run integration tests

---

## Emergency Reference Points

**If you can't find a reference**: Search ItemFactory.cpp for:
- `game.inventory_data` (24 matches)
- `game.d.` (13 matches)
- `game.level_manager` (5 matches)
- `game.log(` (4 matches)
- `game.map` (1 match)

**If you get compiler errors**:
1. Check lambda capture: `[]` should be `[this]`
2. Check method signature: Add `GameContext& ctx` parameter
3. Check pointer syntax: `ctx->` vs `ctx.`
4. Check includes: Add `#include "../Core/GameContext.h"`

**If code compiles but crashes**:
1. Add assertion in init(): Check `ctx_ptr != nullptr`
2. Check Map::init() calls itemFactory->init(ctx)
3. Verify call sites pass ctx parameter correctly

---

## Success Indicators

✓ **Green flags for successful refactoring**:
- [ ] 0 compilation errors
- [ ] grep for `game.` returns 0 results in ItemFactory files
- [ ] grep for `extern Game game` returns 0 results
- [ ] All tests pass
- [ ] Item spawning works as before
- [ ] Logging works correctly
- [ ] No null pointer crashes
- [ ] Code review passes
- [ ] Integration tests pass

This reference guide should accelerate the refactoring process and serve as a checklist during implementation.
