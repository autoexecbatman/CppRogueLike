# Global `extern Game game;` Removed Successfully

## What Was Done

### 1. GameContext.h - Complete Structure ✅
Added all missing members to GameContext:
- `std::deque<std::unique_ptr<BaseMenu>>* menus` - UI menu collection
- Added `#include <deque>` header
- Added `class BaseMenu;` forward declaration

### 2. Game::get_context() - Full Population ✅
Updated `Game.cpp` line 88 to populate:
```cpp
// UI Collections
ctx.menus = &menus;
```

### 3. Removed Global Declaration ✅
**Deleted from Game.h line 159:**
```cpp
extern Game game;  // REMOVED
```

### 4. Fixed Test Files ✅
**tests/Factories/ItemCreatorTest.cpp:**
- Removed `extern Game game;`
- Added `Game testGame;` member to test fixture
- Changed `game.get_context()` → `testGame.get_context()`

## Current State

### Build Status
❌ **Intentionally Failing** - Compiler now shows every location needing refactoring

### Error Count
Approximately **50-100 compilation errors** across multiple files showing:
- `error C2065: 'game': undeclared identifier`
- Each error marks a location that needs `GameContext& ctx` parameter

### Files with Errors (Initial Wave)
1. **src/Items/Armor.cpp** - 3 errors
2. **src/ActorTypes/Monsters/Spider.cpp** - 6 errors
3. **src/ActorTypes/Player.cpp** - 10+ errors
4. More files to be discovered as you fix...

## How to Fix Each Error

### Pattern 1: Constructor/Init Calls
**Error:**
```cpp
auto ctx = game.get_context();  // ERROR: 'game' undeclared
```

**Fix Options:**
```cpp
// Option A: Pass GameContext to function
void init(GameContext& ctx) {
    ctx.map->do_something();
}

// Option B: If in constructor, receive ctx as parameter
Spider(Vector2D pos, GameContext& ctx)
    : Creature(pos)
{
    init_spider_type(ctx);
}
```

### Pattern 2: Direct Global Access
**Error:**
```cpp
game.message("text");  // ERROR: 'game' undeclared
```

**Fix:**
```cpp
// Add GameContext parameter to function
void some_function(GameContext& ctx) {
    ctx.message_system->message(WHITE_BLACK_PAIR, "text", true);
}
```

### Pattern 3: Collections
**Error:**
```cpp
for (auto& item : game.inventory_data.items)  // ERROR
```

**Fix:**
```cpp
void some_function(GameContext& ctx) {
    for (auto& item : ctx.game->inventory_data.items)  // Temporary
    // OR better (when inventory_data added to ctx):
    for (auto& item : ctx.inventory_data->items)
}
```

## Common Replacements

| Old Pattern | New Pattern | Notes |
|-------------|-------------|-------|
| `game.map.X` | `ctx.map->X` | Map already in ctx |
| `game.player->X` | `ctx.player->X` | Player already in ctx |
| `game.d.roll(...)` | `ctx.dice->roll(...)` | RandomDice already in ctx |
| `game.message(...)` | `ctx.message_system->message(...)` | MessageSystem in ctx |
| `game.log(...)` | `ctx.message_system->log(...)` | MessageSystem in ctx |
| `game.creatures` | `ctx.creatures` (deref: `*ctx.creatures`) | Creatures in ctx |
| `game.objects` | `ctx.objects` (deref: `*ctx.objects`) | Objects in ctx |
| `game.inventory_data` | `ctx.game->inventory_data` (temp) | Use ctx.inventory_data when added |
| `game.menus` | `ctx.menus` (deref: `*ctx.menus`) | Menus now in ctx |
| `game.stairs` | `ctx.game->stairs` (temp) | Use ctx.stairs when needed |
| `game.gameStatus` | `ctx.game->gameStatus` (temp) | Game state temporary |
| `game.run` | `ctx.game->run` (temp) | Game state temporary |

## Members Already Available in GameContext

```cpp
struct GameContext {
    // Core Systems
    Map* map;
    Gui* gui;
    Player* player;
    MessageSystem* message_system;
    RandomDice* dice;

    // Managers
    CreatureManager* creature_manager;
    LevelManager* level_manager;
    RenderingManager* rendering_manager;
    InputHandler* input_handler;
    GameStateManager* state_manager;
    MenuManager* menu_manager;
    DisplayManager* display_manager;
    GameLoopCoordinator* game_loop_coordinator;
    DataManager* data_manager;

    // Systems
    TargetingSystem* targeting;
    HungerSystem* hunger_system;

    // Collections
    std::vector<std::unique_ptr<Object>>* objects;
    std::vector<std::unique_ptr<Creature>>* creatures;
    std::deque<std::unique_ptr<BaseMenu>>* menus;  // NEW!
    InventoryData* inventory_data;
    std::vector<Vector2D>* rooms;

    // Special Objects
    Stairs* stairs;

    // Game State
    int* time;
    bool* run;
    int* game_status;

    // Temporary (for migration)
    Game* game;  // Use ctx.game->X for things not yet in ctx
};
```

## Workflow

### For Each Compilation Error:

1. **Open the file** with the error
2. **Find the function** containing `game.X` reference
3. **Add `GameContext& ctx` parameter** to function signature
4. **Replace `game.X`** with appropriate `ctx.X` pattern
5. **Update call sites** to pass ctx
6. **Rebuild** to find next errors
7. **Repeat**

### Tips

- **Work file by file** - Fix all errors in one file before moving to next
- **Start with simplest files** - Armor.cpp (3 errors) is a good start
- **Update headers too** - Don't forget to add GameContext& ctx to .h files
- **Dereference when needed** - Collections are pointers: `*ctx.creatures` not `ctx.creatures`
- **Use ctx.game-> temporarily** - For members not yet directly in GameContext

## Example: Fixing Armor.cpp

**Before:**
```cpp
void Armor::equip(Creature& owner) {
    game.message(WHITE_BLACK_PAIR, "Equipped armor", true);
}
```

**After:**
```cpp
void Armor::equip(Creature& owner, GameContext& ctx) {
    ctx.message_system->message(WHITE_BLACK_PAIR, "Equipped armor", true);
}
```

**Header Update:**
```cpp
// Armor.h
void equip(Creature& owner, GameContext& ctx);  // Add ctx parameter
```

## Next Steps

1. **Start with Armor.cpp** (3 errors - easiest)
2. **Move to Spider.cpp** (6 errors)
3. **Tackle Player.cpp** (10+ errors - most complex)
4. **Continue through remaining files** as compiler reveals them

## Expected Total Effort

- **Total files to fix**: ~50-70 files
- **Total references**: ~1,300-1,500
- **Time per file**: 5-15 minutes
- **Total time**: 8-15 hours (can parallelize with multiple sessions)

## Success Criteria

✅ Build completes with 0 errors
✅ No more `game.` references in codebase (except ctx.game-> temporary usage)
✅ All tests pass
✅ GameContext fully replaces global game object

---

**Status**: Ready for human refactoring work
**Next File**: src/Items/Armor.cpp (3 errors)
