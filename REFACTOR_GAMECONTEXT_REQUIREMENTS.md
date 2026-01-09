# ItemFactory Refactoring: GameContext Member Requirements

## GameContext Members Required

The refactored ItemFactory will require access to the following GameContext members:

### 1. Random Number Generation
**Member**: `RandomDice* dice`
**Location in GameContext.h**: Line 39
**Current Status**: ✓ EXISTS

```cpp
RandomDice* dice{ nullptr };
```

**Used For**:
- `ctx.dice->roll(min, max)` - 9 calls in generate_treasure()
- `ctx.dice->d100()` - 2 calls in generate_treasure()
- `ctx.dice->roll(1, totalWeight)` - 2 calls in category/random spawning

**Total Dice Operations**: 13 replacements

---

### 2. Inventory Management
**Member**: `InventoryData* inventory_data`
**Location in GameContext.h**: Line 59
**Current Status**: ✓ EXISTS

```cpp
class InventoryData* inventory_data{ nullptr };
```

**Used For**:
- `add_item(ctx.inventory_data, item)` - 23 calls throughout constructor lambdas
- `add_item(ctx.inventory_data, goldPile)` - 1 call in generate_treasure()

**Total Inventory Operations**: 24 replacements

---

### 3. Level Management
**Member**: `LevelManager* level_manager`
**Location in GameContext.h**: Line 43
**Current Status**: ✓ EXISTS

```cpp
LevelManager* level_manager{ nullptr };
```

**Used For**:
- `ctx.level_manager->get_dungeon_level()` - 5 calls in constructor lambdas

**Total Level Operations**: 5 replacements

---

### 4. Message/Logging System
**Member**: `MessageSystem* message_system`
**Location in GameContext.h**: Line 38
**Current Status**: ✓ EXISTS

```cpp
MessageSystem* message_system{ nullptr };
```

**Used For**:
- `ctx.message_system->log(message)` - 4 calls:
  - 1 in generate_treasure() (line 273)
  - 1 in spawn_item_of_category() (line 313)
  - 1 in spawn_item_of_category() (line 333)
  - 1 in spawn_random_item() (line 370)

**Total Logging Operations**: 4 replacements

---

### 5. Map/World State
**Member**: `Map* map`
**Location in GameContext.h**: Line 33
**Current Status**: ✓ EXISTS

```cpp
Map* map{ nullptr };
```

**Used For**:
- `ctx.map->can_walk(itemPos)` - 1 call in generate_treasure() (line 229)

**Total Map Operations**: 1 replacement

---

## GameContext Completeness Check

```cpp
struct GameContext {
    // Core game world
    Map* map{ nullptr };                        // ✓ USED (1 ref)
    Gui* gui{ nullptr };                        // ✗ Not used
    Player* player{ nullptr };                  // ✗ Not used

    // Core systems
    MessageSystem* message_system{ nullptr };   // ✓ USED (4 refs)
    RandomDice* dice{ nullptr };                // ✓ USED (13 refs)

    // Managers
    CreatureManager* creature_manager{ nullptr }; // ✗ Not used
    LevelManager* level_manager{ nullptr };     // ✓ USED (5 refs)
    RenderingManager* rendering_manager{ nullptr }; // ✗ Not used
    InputHandler* input_handler{ nullptr };     // ✗ Not used
    GameStateManager* state_manager{ nullptr }; // ✗ Not used
    MenuManager* menu_manager{ nullptr };       // ✗ Not used
    DisplayManager* display_manager{ nullptr }; // ✗ Not used
    GameLoopCoordinator* game_loop_coordinator{ nullptr }; // ✗ Not used
    DataManager* data_manager{ nullptr };       // ✗ Not used

    // Specialized systems
    TargetingSystem* targeting{ nullptr };      // ✗ Not used
    HungerSystem* hunger_system{ nullptr };     // ✗ Not used

    // Game world data
    Stairs* stairs{ nullptr };                  // ✗ Not used
    std::vector<std::unique_ptr<Object>>* objects{ nullptr }; // ✗ Not used
    InventoryData* inventory_data{ nullptr };   // ✓ USED (24 refs)
    std::vector<std::unique_ptr<Creature>>* creatures{ nullptr }; // ✗ Not used

    // Game state
    int* time{ nullptr };                       // ✗ Not used
    bool* run{ nullptr };                       // ✗ Not used
    int* game_status{ nullptr };                // ✗ Not used
};
```

**Summary**:
- Total members in GameContext: 25
- Members used by ItemFactory: 5
- Members unused: 20
- Status: ✓ **ALL REQUIRED MEMBERS EXIST**

---

## Required GameContext Members Reference

### Complete List
```cpp
// Required by ItemFactory (no additions needed to GameContext)
RandomDice* dice;
MessageSystem* message_system;
LevelManager* level_manager;
InventoryData* inventory_data;
Map* map;
```

### Null Safety Requirements

The ItemFactory refactoring should include safety checks:

```cpp
// In ItemFactory::init()
void ItemFactory::init(GameContext& ctx)
{
    ctx_ptr = &ctx;

    // DEBUG: Verify all required pointers exist
    assert(ctx_ptr->inventory_data != nullptr);  // 24 uses
    assert(ctx_ptr->dice != nullptr);            // 13 uses
    assert(ctx_ptr->message_system != nullptr);  // 4 uses
    assert(ctx_ptr->level_manager != nullptr);   // 5 uses
    assert(ctx_ptr->map != nullptr);             // 1 use
}

// In ItemFactory::generate_treasure()
void ItemFactory::generate_treasure(GameContext& ctx, Vector2D position, int dungeonLevel, int quality)
{
    // ASSERT or VALIDATE context
    if (!ctx.dice || !ctx.inventory_data || !ctx.message_system) {
        // Handle error or early return
        return;
    }
    // ... rest of method
}
```

---

## Current GameContext Implementation

### From: `src/Core/GameContext.h`

```cpp
#pragma once

// Forward declarations
class Map;
class Gui;
class MessageSystem;
class RandomDice;
class CreatureManager;
class LevelManager;
class RenderingManager;
class InputHandler;
class GameStateManager;
class MenuManager;
class DisplayManager;
class GameLoopCoordinator;
class DataManager;
class TargetingSystem;
class HungerSystem;
class Player;
class Stairs;
class Object;

/**
 * GameContext - Dependency injection container
 *
 * Replaces global `extern Game game;` with explicit dependency passing.
 * Phase 1: Expand structure with all Game systems ✓ COMPLETE
 * Phase 2: Add get_context() to Game class ✓ COMPLETE
 * Phase 3: Replace game.X with ctx->X incrementally (1,196 references total)
 */
struct GameContext {
    // Core game world
    Map* map{ nullptr };
    Gui* gui{ nullptr };
    Player* player{ nullptr };

    // Core systems
    MessageSystem* message_system{ nullptr };
    RandomDice* dice{ nullptr };

    // Managers
    CreatureManager* creature_manager{ nullptr };
    LevelManager* level_manager{ nullptr };
    RenderingManager* rendering_manager{ nullptr };
    InputHandler* input_handler{ nullptr };
    GameStateManager* state_manager{ nullptr };
    MenuManager* menu_manager{ nullptr };
    DisplayManager* display_manager{ nullptr };
    GameLoopCoordinator* game_loop_coordinator{ nullptr };
    DataManager* data_manager{ nullptr };

    // Specialized systems
    TargetingSystem* targeting{ nullptr };
    HungerSystem* hunger_system{ nullptr };

    // Game world data
    Stairs* stairs{ nullptr };
    std::vector<std::unique_ptr<Object>>* objects{ nullptr };
    class InventoryData* inventory_data{ nullptr };
    std::vector<std::unique_ptr<class Creature>>* creatures{ nullptr };

    // Game state (pointer to allow mutation)
    int* time{ nullptr };
    bool* run{ nullptr };
    int* game_status{ nullptr };  // Pointer to Game::GameStatus enum
};
```

---

## Initialization in Game Class

### From: `src/Game.cpp` (get_context() method)

The Game class must initialize GameContext with valid pointers. The initialization should occur in `get_context()`:

```cpp
GameContext Game::get_context() noexcept {
    return GameContext{
        .map = &map,
        .gui = &gui,
        .player = player.get(),
        .message_system = &message_system,
        .dice = &d,
        .creature_manager = &creature_manager,
        .level_manager = &level_manager,
        .rendering_manager = &rendering_manager,
        .input_handler = &input_handler,
        .state_manager = &state_manager,
        .menu_manager = &menu_manager,
        .display_manager = &display_manager,
        .game_loop_coordinator = &game_loop_coordinator,
        .data_manager = &data_manager,
        .targeting = &targeting,
        .hunger_system = &hunger_system,
        .stairs = stairs.get(),
        .objects = &objects,
        .inventory_data = &inventory_data,
        .creatures = &creatures,
        .time = &time,
        .run = &run,
        .game_status = reinterpret_cast<int*>(&gameStatus)
    };
}
```

---

## Diagram: Member Access Flow

```
ItemFactory Methods
├── constructor()
│   └── Lambda functions
│       └── ctx_ptr->inventory_data        (store in member)
│       └── ctx_ptr->level_manager         (store in member)
│
├── generate_treasure(ctx, ...)
│   ├── ctx.dice->roll()                   (9 calls)
│   ├── ctx.dice->d100()                   (2 calls)
│   ├── ctx.inventory_data                 (1 call)
│   ├── ctx.message_system->log()          (1 call)
│   ├── ctx.map->can_walk()                (1 call)
│   └── spawn_item_of_category(ctx, ...)   (6 calls)
│
├── spawn_item_of_category(ctx, ...)
│   ├── ctx.dice->roll()                   (1 call)
│   └── ctx.message_system->log()          (2 calls)
│
└── spawn_random_item(ctx, ...)
    ├── ctx.dice->roll()                   (1 call)
    └── ctx.message_system->log()          (1 call)
```

---

## Compatibility with Existing Code

### Game::get_context() Availability

The `get_context()` method should be available by the time:
1. Map::init() is called (where itemFactory is initialized)
2. Map::spawn_items() is called
3. Map::generate_treasure() is called

All three occur AFTER Game::init() completes, so context will be valid.

### Forward Declaration Requirement

Add to ItemFactory.h:
```cpp
struct GameContext;  // Forward declaration for dependency injection
```

This avoids circular includes:
- ItemFactory.h includes GameContext.h would be circular if GameContext needed ItemFactory
- Forward declaration allows ItemFactory to accept GameContext& parameter

---

## Summary Table

| Member | Type | ItemFactory Uses | Notes |
|--------|------|------------------|-------|
| `dice` | `RandomDice*` | 13 references | Dice rolls for randomization |
| `inventory_data` | `InventoryData*` | 24 references | Add items to floor |
| `level_manager` | `LevelManager*` | 5 references | Get dungeon level for enhancement |
| `message_system` | `MessageSystem*` | 4 references | Log messages |
| `map` | `Map*` | 1 reference | Check walkability |

**Total Impact**: 47 references → 0 `game.` references after refactoring

---

## No Additional Changes Needed

✓ **GameContext already contains all required members**
✓ **No modifications to GameContext.h needed**
✓ **No modifications to Game class needed** (get_context() already exists)
✓ **No new dependencies introduced**

The refactoring is purely additive to ItemFactory and Map classes.
