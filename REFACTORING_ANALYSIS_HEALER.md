# Healer Class GameContext Dependency Injection Refactoring Analysis

**Agent**: Agent-Actor-02
**Date**: 2026-01-08
**Status**: Analysis Only (No Code Changes)

---

## Executive Summary

The `Healer` class (src/ActorTypes/Healer.h and .cpp) currently depends on the global `extern Game game;` object for messaging functionality. This analysis documents all required changes to refactor it to use GameContext dependency injection pattern.

**Key Finding**: The Healer class is MINIMAL in scope - it only makes 4 game references (all in the `use()` method), making it an ideal candidate for refactoring.

---

## 1. Current Game References Found

### Location: `src/ActorTypes/Healer.cpp` - `use()` method (lines 8-26)

**Total References**: 4 calls to `game.message()`

```cpp
// Line 14: Healer::use() - First message call
game.message(COLOR_WHITE, "You heal ", false);

// Line 15: Healer::use() - Second message call (continuation)
game.message(COLOR_RED, std::to_string(amountHealed), false);

// Line 16: Healer::use() - Third message call (completion)
game.message(COLOR_WHITE, " hit points.", true);

// Line 22: Healer::use() - Fourth message call (failure case)
game.message(COLOR_RED, "Health is already maxed out!", true);
```

**Complete `use()` method context**:
```cpp
bool Healer::use(Item& owner, Creature& wearer)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		game.message(COLOR_WHITE, "You heal ", false);
		game.message(COLOR_RED, std::to_string(amountHealed), false);
		game.message(COLOR_WHITE, " hit points.", true);

		return Pickable::use(owner, wearer);
	}
	else
	{
		game.message(COLOR_RED, "Health is already maxed out!", true);
	}

	return false;
}
```

**Other methods**: `load()`, `save()`, `get_type()` do NOT reference `game`.

---

## 2. Updated Method Signatures

### Current Signatures

**Header (Healer.h)**:
```cpp
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 };

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;
};
```

### Proposed Signatures

**Header (Healer.h)**:
```cpp
// Forward declaration (add at top of file)
struct GameContext;

class Healer : public Pickable
{
public:
	int amountToHeal{ 0 };

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer) override;
	// New method signature with GameContext
	bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;
};
```

**Implementation (Healer.cpp)**:
```cpp
bool Healer::use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		ctx.message_system->message(COLOR_WHITE, "You heal ", false);
		ctx.message_system->message(COLOR_RED, std::to_string(amountHealed), false);
		ctx.message_system->message(COLOR_WHITE, " hit points.", true);

		return Pickable::use(owner, wearer);
	}
	else
	{
		ctx.message_system->message(COLOR_RED, "Health is already maxed out!", true);
	}

	return false;
}
```

**Alternative: Replace the base method entirely**:
```cpp
bool Healer::use(Item& owner, Creature& wearer) override
{
	// This would require virtual method signature change in Pickable
	// NOT RECOMMENDED - breaks Liskov Substitution Principle
}
```

---

## 3. Pickable Interface Constraints

### Problem: Base Class Interface Limitation

**Current Pickable base class signature** (Pickable.h, line 67):
```cpp
virtual bool use(Item& owner, Creature& wearer);
```

**Challenge**: The `use()` method signature is defined in the Pickable base class without GameContext parameter. Options:

### Option A: Extend Pickable with New Virtual Method (RECOMMENDED)
```cpp
class Pickable : public Persistent
{
public:
	virtual bool use(Item& owner, Creature& wearer);  // Keep for compatibility

	// New method - implementations override this
	virtual bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
	{
		// Default: call the old use() method
		return use(owner, wearer);
	}
};
```

**Advantages**:
- Backward compatible
- Allows gradual migration
- Doesn't break existing code
- Inheriting classes can override either method

**Disadvantages**:
- Creates two parallel APIs temporarily

### Option B: Replace Base Method Signature (NOT RECOMMENDED)
Change `Pickable::use()` to include GameContext parameter:
```cpp
virtual bool use(Item& owner, Creature& wearer, GameContext& ctx);
```

**Disadvantages**:
- Requires refactoring ALL derived classes at once
- Breaks LSP if not all implementations are updated simultaneously
- Higher risk of inconsistent state

---

## 4. Call Sites Needing Updates

### Primary Call Site: InventoryUI.cpp (line 317)

**File**: `src/UI/InventoryUI.cpp`
**Method**: `InventoryUI::handle_backpack_selection()`
**Current Code** (line 317):
```cpp
bool itemUsed = selectedItem->pickable->use(*selectedItem, player);
```

**Updated Code**:
```cpp
// Requires GameContext parameter
auto ctx = game.get_context();  // or passed as parameter
bool itemUsed = selectedItem->pickable->use_with_context(*selectedItem, player, ctx);
```

**Secondary usage locations** (from grep results):
- `src/Items/*.cpp.test.cpp` - Test files (6 files) - Use test contexts or mocks
- Base `Pickable::use()` calls in other derived classes

### Current Dependency Flow

```
InventoryUI::handle_backpack_selection()
    └─> Item::pickable->use(*selectedItem, player)
            └─> Healer::use()
                    ├─> game.message()  ← NEEDS GameContext
                    └─> game.get_actor() / game.pick_tile() (in other classes)
```

**New Dependency Flow**:
```
InventoryUI::handle_backpack_selection()
    └─> GameContext ctx = game.get_context()
    └─> Item::pickable->use_with_context(*selectedItem, player, ctx)
            └─> Healer::use_with_context()
                    └─> ctx.message_system->message()  ← Injected dependency
```

---

## 5. Required GameContext Members

### Members Used by Healer

From analysis of `Healer.cpp`, the following GameContext members are needed:

```cpp
struct GameContext {
    // REQUIRED FOR HEALER:
    MessageSystem* message_system{ nullptr };

    // For potential future use in similar classes:
    // Creature* get_closest_monster(Vector2D, double)  // Used by LightningBolt
    // Creature* get_actor(Vector2D)                     // Used by Confuser
    // Map* map                                          // Used by LightningBolt animations
    // bool pick_tile(Vector2D*, int)                   // Used by Confuser
};
```

### GameContext Member Status

**Current GameContext.h** (lines 31-66):
```cpp
struct GameContext {
    // Core game world
    Map* map{ nullptr };
    Gui* gui{ nullptr };
    Player* player{ nullptr };

    // Core systems
    MessageSystem* message_system{ nullptr };    // ✓ ALREADY PRESENT
    RandomDice* dice{ nullptr };

    // Managers (... others ...)

    // Game world data (... others ...)
};
```

**Status**: MessageSystem pointer is ALREADY defined in GameContext ✓

---

## 6. Implementation Strategy

### Phase 1: Add New Virtual Method to Pickable (SAFE)

**File**: `src/Actor/Pickable.h`

**Add forward declaration**:
```cpp
struct GameContext;
```

**Add new virtual method**:
```cpp
class Pickable : public Persistent
{
public:
	// ... existing code ...

	// New dependency-injected version
	virtual bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
	{
		// Default implementation delegates to old use()
		return use(owner, wearer);
	}
};
```

### Phase 2: Refactor Healer (ISOLATED)

**File**: `src/ActorTypes/Healer.h`

**Add forward declaration**:
```cpp
struct GameContext;
```

**Add new method**:
```cpp
bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx) override;
```

**File**: `src/ActorTypes/Healer.cpp`

**Implement new method** (lines 28-48):
```cpp
bool Healer::use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		ctx.message_system->message(COLOR_WHITE, "You heal ", false);
		ctx.message_system->message(COLOR_RED, std::to_string(amountHealed), false);
		ctx.message_system->message(COLOR_WHITE, " hit points.", true);

		return Pickable::use(owner, wearer);
	}
	else
	{
		ctx.message_system->message(COLOR_RED, "Health is already maxed out!", true);
	}

	return false;
}
```

### Phase 3: Update Call Sites (REQUIRES COORDINATION)

**File**: `src/UI/InventoryUI.cpp` (line 317)

**Current**:
```cpp
bool itemUsed = selectedItem->pickable->use(*selectedItem, player);
```

**Updated**:
```cpp
auto ctx = game.get_context();
bool itemUsed = selectedItem->pickable->use_with_context(*selectedItem, player, ctx);
```

**OR pass GameContext from caller**:
```cpp
// If InventoryUI::handle_backpack_selection receives GameContext& ctx:
bool itemUsed = selectedItem->pickable->use_with_context(*selectedItem, player, ctx);
```

### Phase 4: Test Updates (REQUIRED)

**Files affected**:
- `tests/Combat/DamageInfoTest.cpp` (new test file)
- `tests/Combat/WeaponDamageRegistryTest.cpp` (new test file)
- Potential test files in `src/Items/*.cpp.test.cpp` (6 untracked test files)

**Mock GameContext creation for tests**:
```cpp
// In test setup
GameContext test_ctx;
MessageSystem test_message_system;
test_ctx.message_system = &test_message_system;

// Then call
healer.use_with_context(item, player, test_ctx);
```

---

## 7. Comparison with Similar Classes

### LightningBolt.cpp Analysis

**Lines using game.**:
- Line 17: `game.get_closest_monster()` - Needs CreatureManager function
- Lines 21, 26-29, 34: `game.message()`, `game.append_message_part()` - MessageSystem
- Line 87: `game.render()` - RenderingManager
- Lines 145, 180: `game.map.get_width()`/`get_height()` - Map access
- Line 165, 192: `game.render()`, `game.restore_game_display()` - RenderingManager

**Total**: 11+ references - More complex refactoring

### Confuser.cpp Analysis

**Lines using game.**:
- Line 15: `game.pick_tile()` - TargetingSystem
- Line 24: `game.get_actor()` - CreatureManager
- Lines 20, 31, 40, 41, 46: `game.restore_game_display()` - RenderingManager
- Line 40: `game.message()` - MessageSystem

**Total**: 8+ references - More complex refactoring

### Healer.cpp Analysis

**Lines using game.**:
- Lines 14-16, 22: `game.message()` - MessageSystem ONLY

**Total**: 4 references - SIMPLEST refactoring

---

## 8. Code Summary

### Files to Modify

| File | Type | Changes | Risk |
|------|------|---------|------|
| `src/Actor/Pickable.h` | Header | Add `struct GameContext` forward declaration + `use_with_context()` virtual method | LOW |
| `src/ActorTypes/Healer.h` | Header | Add `struct GameContext` forward declaration + `use_with_context()` declaration | LOW |
| `src/ActorTypes/Healer.cpp` | Implementation | Implement `use_with_context()` method | LOW |
| `src/UI/InventoryUI.cpp` | Call Site | Update line 317 to use `use_with_context()` | MEDIUM |
| Test files | Tests | Update any tests calling Healer::use() | MEDIUM |

### Files NOT Modified

- `src/Core/GameContext.h` - Already has `MessageSystem* message_system`
- `src/Game.h` - Already has `get_context()` method
- `src/ActorTypes/Healer.cpp` - Keep original `use()` method for compatibility

---

## 9. Reference Implementation Pattern

Based on existing `Game::get_context()` (Game.h, line 108):

```cpp
GameContext Game::get_context() noexcept {
    return GameContext{
        .map = &map,
        .gui = &gui,
        .player = player.get(),
        // ... all members ...
        .message_system = &message_system,
        // ... etc ...
    };
}
```

This pattern is already implemented and working in the Game class.

---

## 10. Detailed Dependencies and Access Patterns

### Current: Global Access Pattern
```cpp
// In Healer::use()
game.message(COLOR_WHITE, "You heal ", false);  // Line 14
// Accesses: game::message_system through Game's delegation method
```

### New: Injected Pattern
```cpp
// In Healer::use_with_context()
ctx.message_system->message(COLOR_WHITE, "You heal ", false);
// Direct access to message_system through GameContext pointer
```

### Message Flow Comparison

**Before (Global)**:
```
Healer::use()
  └─ extern Game game
     └─ game.message()
        └─ game.message_system.message()
```

**After (Injected)**:
```
Healer::use_with_context(GameContext& ctx)
  └─ ctx.message_system->message()
```

---

## 11. Validation Checklist

Before implementing, verify:

- [ ] GameContext has MessageSystem* member (YES - line 38 in GameContext.h)
- [ ] Game::get_context() is implemented (YES - line 108 in Game.h)
- [ ] Pickable base class can be extended with new virtual (YES - it's designed for polymorphism)
- [ ] All test infrastructure supports GameContext mocking (PARTIAL - needs implementation)
- [ ] InventoryUI has access to game.get_context() (YES - it includes Game.h)
- [ ] No other code in Healer depends on Game except use() (YES - verified in code review)

---

## 12. Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Breaking existing Pickable subclasses | HIGH | Use new virtual method, keep old one as default implementation |
| Test infrastructure not ready for GameContext | MEDIUM | Create simple GameContext test wrapper with mocked MessageSystem |
| InventoryUI call site not updated | HIGH | Coordinate update to call site as part of same commit |
| Forgetting to include GameContext.h | MEDIUM | Add to Healer.h as part of refactoring |
| Multiple inheritance diamond | LOW | Use virtual inheritance or composition (already handled in codebase) |

---

## 13. Success Criteria

- [ ] Healer.h compiles with GameContext forward declaration
- [ ] Healer::use_with_context() compiles and runs
- [ ] All Healer message calls use ctx.message_system->message()
- [ ] Original Healer::use() method still works (for backward compatibility)
- [ ] InventoryUI properly calls use_with_context()
- [ ] No direct `game.` references in Healer.cpp (except includes)
- [ ] All tests pass with injected GameContext
- [ ] Code follows existing patterns in Game and GameContext

---

## Conclusion

The Healer class is an excellent refactoring candidate due to:
1. **Small scope** - Only 4 game references, all in one method
2. **Simple dependency** - Only needs MessageSystem, already in GameContext
3. **Low risk** - Isolated changes, can be tested independently
4. **Pattern alignment** - Follows established GameContext design

The refactoring can be implemented safely using the Phase 1-4 strategy without breaking existing code, and serves as a template for refactoring similar classes (Confuser, LightningBolt, etc.).

