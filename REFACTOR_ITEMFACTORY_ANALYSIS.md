# ItemFactory Refactoring Analysis: GameContext Dependency Injection

## Executive Summary

The `ItemFactory` class currently uses **9 extern `game.` references** that need to be refactored to accept `GameContext&` dependency injection parameters. This refactoring eliminates global coupling and enables better testability and dependency management.

---

## 1. ALL `game.` REFERENCES FOUND

### ItemFactory.cpp (9 total references)

| Line | Reference | System | Purpose |
|------|-----------|--------|---------|
| 21 | `game.inventory_data` | Inventory System | Add created health potion to floor items |
| 29 | `game.inventory_data` | Inventory System | Add created scroll (lightning) to floor items |
| 36 | `game.inventory_data` | Inventory System | Add created scroll (fireball) to floor items |
| 43 | `game.inventory_data` | Inventory System | Add created scroll (confusion) to floor items |
| 51 | `game.level_manager.get_dungeon_level()` | Level Manager | Get current dungeon level for enhancement |
| 51 | `game.inventory_data` | Inventory System | Add enhanced dagger to floor items |
| 58 | `game.level_manager.get_dungeon_level()` | Level Manager | Get current dungeon level for enhancement |
| 58 | `game.inventory_data` | Inventory System | Add enhanced short sword to floor items |
| 65 | `game.level_manager.get_dungeon_level()` | Level Manager | Get current dungeon level for enhancement |
| 65 | `game.inventory_data` | Inventory System | Add enhanced long sword to floor items |
| 72 | `game.level_manager.get_dungeon_level()` | Level Manager | Get current dungeon level for enhancement |
| 72 | `game.inventory_data` | Inventory System | Add enhanced staff to floor items |
| 79 | `game.level_manager.get_dungeon_level()` | Level Manager | Get current dungeon level for enhancement |
| 79 | `game.inventory_data` | Inventory System | Add enhanced longbow to floor items |
| 86 | `game.inventory_data` | Inventory System | Add gold pile to floor items |
| 92 | `game.inventory_data` | Inventory System | Add ration to floor items |
| 97 | `game.inventory_data` | Inventory System | Add fruit to floor items |
| 102 | `game.inventory_data` | Inventory System | Add bread to floor items |
| 107 | `game.inventory_data` | Inventory System | Add meat to floor items |
| 113 | `game.inventory_data` | Inventory System | Add amulet of Yendor to floor items |
| 119 | `game.inventory_data` | Inventory System | Add leather armor to floor items |
| 126 | `game.inventory_data` | Inventory System | Add chain mail to floor items |
| 133 | `game.inventory_data` | Inventory System | Add plate mail to floor items |
| **188** | `game.d.roll(1, 2)` | RandomDice | Roll dice for item count (quality=1) |
| **194** | `game.d.roll(2, 3)` | RandomDice | Roll dice for item count (quality=2) |
| **200** | `game.d.roll(3, 5)` | RandomDice | Roll dice for item count (quality=3) |
| **213** | `game.d.roll(goldMin, goldMax)` | RandomDice | Roll gold pile amount |
| **218** | `game.inventory_data` | Inventory System | Add generated gold pile to floor |
| **225** | `game.d.roll(-1, 1)` | RandomDice | Random item position offset X |
| **226** | `game.d.roll(-1, 1)` | RandomDice | Random item position offset Y |
| **229** | `game.map.can_walk(itemPos)` | Map System | Check if position is walkable |
| **235** | `game.d.d100()` | RandomDice | Roll d100 for treasure quality |
| **240** | `game.d.d100()` | RandomDice | Roll d100 for artifact check |
| **273** | `game.log(...)` | Message System | Log treasure generation |
| **313** | `game.log(...)` | Message System | Log category not found |
| **333** | `game.log(...)` | Message System | Log no valid items warning |
| **338** | `game.d.roll(1, totalWeight)` | RandomDice | Roll for item selection |
| **370** | `game.log(...)` | Message System | Log no valid items warning |
| **375** | `game.d.roll(1, totalWeight)` | RandomDice | Roll for item selection |

**Summary by System:**
- **Inventory System**: 23 references to `game.inventory_data`
- **RandomDice**: 10 references to `game.d.*`
- **Message System**: 4 references to `game.log()`
- **Level Manager**: 5 references to `game.level_manager.get_dungeon_level()`
- **Map System**: 1 reference to `game.map.can_walk()`

---

## 2. REQUIRED GAMECONTEXT MEMBERS

Based on the analysis, refactored code will require access to:

```cpp
struct GameContext {
    InventoryData* inventory_data;      // For adding items to floor
    RandomDice* dice;                   // For rolling d100, roll(min,max), d100()
    LevelManager* level_manager;        // For get_dungeon_level()
    MessageSystem* message_system;      // For logging via game.log()
    Map* map;                           // For map.can_walk() validation
};
```

All these members are **already present in GameContext.h** (lines 39, 43, 59):
- ✓ `RandomDice* dice{ nullptr };` (line 39)
- ✓ `LevelManager* level_manager{ nullptr };` (line 43)
- ✓ `InventoryData* inventory_data{ nullptr };` (line 59)
- ✓ `MessageSystem* message_system{ nullptr };` (line 38)
- ✓ `Map* map{ nullptr };` (line 33)

---

## 3. UPDATED METHOD SIGNATURES

### Current (with extern game) → Refactored (with GameContext)

#### Constructor
```cpp
// Current
ItemFactory::ItemFactory()

// Refactored - Add dependency parameter
ItemFactory::ItemFactory(GameContext& ctx)
```

#### Public Methods

**`generate_treasure`**
```cpp
// Current
void ItemFactory::generate_treasure(Vector2D position, int dungeonLevel, int quality)

// Refactored
void ItemFactory::generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality)
```

**`spawn_item_of_category`**
```cpp
// Current
void ItemFactory::spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category)

// Refactored
void ItemFactory::spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category)
```

**`spawn_random_item`**
```cpp
// Current
void ItemFactory::spawn_random_item(Vector2D position, int dungeonLevel)

// Refactored
void ItemFactory::spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel)
```

#### Private Methods (Internal Use)

**`calculate_weight`** - No changes needed
```cpp
// No game references
int ItemFactory::calculate_weight(const ItemType& item, int dungeonLevel) const
```

**`add_item_type`** - No changes needed
```cpp
// No game references
void ItemFactory::add_item_type(const ItemType& itemType)
```

**`get_current_distribution`** - No changes needed
```cpp
// No game references
std::vector<ItemPercentage> ItemFactory::get_current_distribution(int dungeonLevel)
```

---

## 4. CALL SITE REFACTORING

### Location: `src/Map/Map.cpp`

#### Current Call Sites (2 locations)

**Call Site 1** (Line 581 in Map.cpp)
```cpp
// Current
itemFactory->spawn_random_item(pos, ctx.level_manager->get_dungeon_level());

// Refactored
itemFactory->spawn_random_item(ctx, pos, ctx.level_manager->get_dungeon_level());
```

**Call Site 2** (Line 1297 in Map.cpp)
```cpp
// Current
itemFactory->generate_treasure(center, ctx.level_manager->get_dungeon_level(), quality);

// Refactored
itemFactory->generate_treasure(ctx, center, ctx.level_manager->get_dungeon_level(), quality);
```

### Location: `src/Factories/ItemFactory.cpp`

#### Constructor Initialization Changes

**Lines 18-135** - Lambda Functions in Constructor
```cpp
// Current lambda pattern
add_item_type({
    "Health Potion", 50, 1, 0, 0.2f,
    [](Vector2D pos) {
        add_item(game.inventory_data, ItemCreator::create_health_potion(pos));
    }
});

// Refactored lambda pattern - PROBLEM: Lambdas cannot access member GameContext
// SOLUTION: Pass inventory_data explicitly or store ctx as member
[this](Vector2D pos) {
    add_item(ctx.inventory_data, ItemCreator::create_health_potion(pos));
}
```

**CRITICAL DESIGN DECISION:** Lambdas in constructor need access to `GameContext`. Two options:
1. **Store GameContext reference as class member** - requires mutable state
2. **Convert lambdas to static methods** - bypasses lambda context limitation
3. **Use std::function with captured context** - requires refactoring Item type struct

---

## 5. DETAILED REFACTORING BREAKDOWN

### Phase 1: Header Changes

**File: `src/Factories/ItemFactory.h`**

```cpp
// Add forward declaration
struct GameContext;

class ItemFactory
{
public:
    // Updated constructor signature
    ItemFactory(GameContext& ctx);

    // Updated public methods
    void generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality);
    void spawn_item_of_category(GameContext& ctx, Vector2D position, int dungeonLevel, const std::string& category);
    void spawn_random_item(GameContext& ctx, Vector2D position, int dungeonLevel);

    // Unchanged
    void add_item_type(const ItemType& itemType);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);

private:
    // NEW: Store context for lambda access (if using option 1)
    GameContext* ctx_ptr = nullptr;

    // Existing members
    std::vector<ItemType> itemTypes;
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;
    int calculate_weight(const ItemType& item, int dungeonLevel) const;
};
```

### Phase 2: Implementation Changes

**File: `src/Factories/ItemFactory.cpp`**

#### Constructor Header
```cpp
// Current
ItemFactory::ItemFactory()

// Refactored
ItemFactory::ItemFactory(GameContext& ctx)
    : ctx_ptr(&ctx)
{
    // ... rest of constructor
}
```

#### Lambda Refactoring (Option 1: Member Variable)
```cpp
// Current
[](Vector2D pos) {
    add_item(game.inventory_data, ItemCreator::create_health_potion(pos));
}

// Refactored
[this](Vector2D pos) {
    add_item(ctx_ptr->inventory_data, ItemCreator::create_health_potion(pos));
}
```

#### Method Implementation Changes

**`generate_treasure` method**
- Replace all `game.d.roll()` with `ctx.dice->roll()`
- Replace all `game.d.d100()` with `ctx.dice->d100()`
- Replace all `game.inventory_data` with `ctx.inventory_data`
- Replace all `game.log()` with `ctx.message_system->log()`
- Replace all `game.map.can_walk()` with `ctx.map->can_walk()`
- Update internal method calls to pass `ctx` parameter

**`spawn_item_of_category` method**
- Replace all `game.d.roll()` with `ctx.dice->roll()`
- Replace all `game.log()` with `ctx.message_system->log()`
- Update calls to `itemTypes[idx].createFunc(position)` to handle ctx access

**`spawn_random_item` method**
- Replace all `game.d.roll()` with `ctx.dice->roll()`
- Replace all `game.log()` with `ctx.message_system->log()`
- Update calls to `itemTypes[i].createFunc(position)` to handle ctx access

---

## 6. CONSTRAINT: ItemType STRUCT AND LAMBDA PROBLEM

### Current ItemType Definition
```cpp
struct ItemType
{
    std::string name;
    int baseWeight;
    int levelMinimum;
    int levelMaximum;
    float levelScaling;
    std::function<void(Vector2D)> createFunc;  // ← Problem: Lambda needs GameContext
};
```

### Problem Analysis

The `createFunc` lambda captures and uses `game.inventory_data`. When refactoring:

**Option A: Add GameContext to ItemType** (Breaking Change)
```cpp
struct ItemType
{
    std::string name;
    int baseWeight;
    int levelMinimum;
    int levelMaximum;
    float levelScaling;
    std::function<void(GameContext&, Vector2D)> createFunc;  // ← Requires all callers update
};
```

**Option B: Store GameContext as ItemFactory member** (Recommended for backward compatibility)
```cpp
// In ItemFactory.h
class ItemFactory
{
private:
    GameContext* ctx_ptr = nullptr;  // Store for lambda access
};

// In lambdas:
[this](Vector2D pos) {
    add_item(ctx_ptr->inventory_data, ItemCreator::create_health_potion(pos));
}
```

**Option C: Use std::bind to inject context into lambdas** (Complex refactoring)
```cpp
auto boundCreateFunc = std::bind([](GameContext& ctx, Vector2D pos) {
    add_item(ctx.inventory_data, ItemCreator::create_health_potion(pos));
}, std::ref(ctx), std::placeholders::_1);
```

### Recommendation: **Option B** (Member Variable)
- Minimal API changes
- Maintains backward compatibility with ItemType struct
- Allows lambdas to access context via member pointer
- Easy to debug and understand

---

## 7. INSTANTIATION SITE CHANGES

### Location: `src/Map/Map.h` (Line 73-74)

Currently, `ItemFactory` is instantiated in Map's constructor WITHOUT parameters:

```cpp
// Current (Line 223-224 in Map.cpp)
Map::Map(...)
    : ...
    monsterFactory(std::make_unique<MonsterFactory>()),
    itemFactory(std::make_unique<ItemFactory>())  // ← No parameters
{
}
```

#### Problem: Map doesn't have GameContext at construction time

**Solution:** Defer initialization or use setter method

**Option 1: Add init method to ItemFactory**
```cpp
class ItemFactory
{
public:
    ItemFactory();  // Keep default constructor
    void init(GameContext& ctx) { ctx_ptr = &ctx; }  // Initialize context later
};

// In Map::init() (called after context is available)
itemFactory->init(ctx);
```

**Option 2: Update Map constructor signature** (Breaking change)
```cpp
// Requires all call sites to be updated
Map(int height, int width, GameContext& ctx);
```

### Recommendation: **Option 1** (Deferred Initialization)
- No breaking changes to Map constructor
- Called in `Map::init()` which already receives GameContext
- Consistent with existing pattern where Map::init takes GameContext

---

## 8. REFERENCE TRACKING: BEFORE AND AFTER

### Before Refactoring
```cpp
// ItemFactory.cpp - 37 total game. references
extern Game game;

// Constructor (23 refs inside lambdas)
game.inventory_data, game.level_manager.get_dungeon_level()

// generate_treasure (11 refs)
game.d.roll(), game.level_manager.get_dungeon_level(), game.inventory_data,
game.d.d100(), game.map.can_walk(), game.log()

// spawn_item_of_category (3 refs)
game.log(), game.d.roll()

// spawn_random_item (3 refs)
game.log(), game.d.roll()
```

### After Refactoring
```cpp
// ItemFactory.cpp - 0 game. references
// No extern Game game;

// Constructor uses: ctx_ptr->inventory_data
// Methods use: ctx.dice->roll(), ctx.map->can_walk(), ctx.message_system->log()
// All global coupling eliminated
```

---

## 9. TEST IMPACT

### Current Test File
**Location:** `src/Factories/ItemFactory.cpp.test.cpp`

Tests will need to:
1. Create a mock/real GameContext
2. Pass it to ItemFactory constructor or init method
3. Update all test method calls to include ctx parameter

### Example Test Refactoring
```cpp
// Current
ItemFactory factory;
factory.spawn_random_item(pos, 5);

// Refactored
GameContext ctx = setup_test_context();  // Helper to create test context
ItemFactory factory;
factory.init(ctx);
factory.spawn_random_item(ctx, pos, 5);
```

---

## 10. IMPLEMENTATION CHECKLIST

### Phase 1: Header Updates
- [ ] Add `struct GameContext;` forward declaration to ItemFactory.h
- [ ] Add `GameContext* ctx_ptr = nullptr;` member variable to ItemFactory
- [ ] Update constructor signature in ItemFactory.h
- [ ] Add `void init(GameContext& ctx)` method to ItemFactory.h
- [ ] Update public method signatures with `GameContext& ctx` parameter

### Phase 2: Implementation Updates
- [ ] Update ItemFactory constructor to store context pointer
- [ ] Add `init()` method implementation
- [ ] Refactor all 23 lambdas in constructor to use `this->ctx_ptr->inventory_data`
- [ ] Update `generate_treasure()` implementation (11 reference replacements)
- [ ] Update `spawn_item_of_category()` implementation (3 reference replacements)
- [ ] Update `spawn_random_item()` implementation (3 reference replacements)

### Phase 3: Call Site Updates
- [ ] Update Map::init() to call `itemFactory->init(ctx)` after construction
- [ ] Update Map::spawn_items() call to `itemFactory->spawn_random_item(ctx, ...)`
- [ ] Update Map::generate_treasure() call to `itemFactory->generate_treasure(ctx, ...)`

### Phase 4: Tests
- [ ] Update ItemFactory.cpp.test.cpp with new GameContext requirements
- [ ] Add integration tests for GameContext passing
- [ ] Verify all 37 game references are eliminated

---

## 11. BACKWARD COMPATIBILITY

### Impact on External Code

**No public API breaking changes** if using deferred initialization strategy:
- Default constructor still exists
- New methods are additive only
- Existing signatures extended with optional parameter after context

### Migration Path for Call Sites
1. Old: `itemFactory->spawn_random_item(pos, level)`
2. New: `itemFactory->spawn_random_item(ctx, pos, level)`

---

## 12. EFFORT ESTIMATE

| Task | Complexity | Effort |
|------|-----------|--------|
| Header declarations | Low | 10 min |
| Constructor refactoring | Medium | 20 min |
| Lambda refactoring (23 refs) | Medium | 30 min |
| Method implementations (17 refs) | High | 45 min |
| Call site updates (2 locations) | Low | 10 min |
| Test updates | Medium | 25 min |
| Integration verification | Medium | 20 min |
| **TOTAL** | **Medium** | **~160 min (2.5 hours)** |

---

## 13. RISK ASSESSMENT

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Lambdas lose context access | High | Store ctx_ptr as member variable |
| Item creation timing issues | Medium | Use deferred init() pattern |
| Test complexity increases | Medium | Create test context helper |
| Map initialization order | Medium | Call init() in Map::init() after construction |
| Forgetting to call init() | Low | Add assertion to spawn methods checking ctx_ptr |

---

## SUMMARY

**Total game. references to eliminate: 37**

The refactoring is straightforward because:
1. All required GameContext members already exist
2. Two public methods are the main entry points
3. Lambdas can be handled with member variable pattern
4. Call sites are limited to Map class (2 locations)

**Recommendation:** Implement using Option B (member variable) + Deferred init() pattern for minimal disruption and maximum compatibility.
