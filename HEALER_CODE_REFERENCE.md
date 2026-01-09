# Healer Refactoring - Complete Code Reference

## Source Files

### Current Healer.h (src/ActorTypes/Healer.h)
```cpp
#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 }; // how many hp

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;
};
//====
```

### Current Healer.cpp (src/ActorTypes/Healer.cpp)
```cpp
#include "Healer.h"
#include "../Game.h"
#include "../Colors/Colors.h"

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Item& owner, Creature& wearer)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		game.message(COLOR_WHITE, "You heal ", false);          // LINE 14 - GAME REF #1
		game.message(COLOR_RED, std::to_string(amountHealed), false);    // LINE 15 - GAME REF #2
		game.message(COLOR_WHITE, " hit points.", true);        // LINE 16 - GAME REF #3

		return Pickable::use(owner, wearer);
	}
	else
	{
		game.message(COLOR_RED, "Health is already maxed out!", true);   // LINE 22 - GAME REF #4
	}

	return false;
}

void Healer::load(const json& j)
{
	if (j.contains("amountToHeal") && j["amountToHeal"].is_number())
	{
		amountToHeal = j["amountToHeal"].get<int>();
	}
	else
	{
		throw std::runtime_error("Invalid JSON format for Healer");
	}
}

void Healer::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::HEALER);
	j["amountToHeal"] = amountToHeal;
}

Pickable::PickableType Healer::get_type() const
{
	return PickableType::HEALER;
}
```

---

## GameContext.h (Relevant Excerpt)
```cpp
struct GameContext {
    // Core game world
    Map* map{ nullptr };
    Gui* gui{ nullptr };
    Player* player{ nullptr };

    // Core systems
    MessageSystem* message_system{ nullptr };  // <<< USED BY HEALER
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
    int* game_status{ nullptr };
};
```

---

## Game::get_context() Implementation (Game.h:108)
```cpp
class Game
{
public:
    // ... other members ...

    // Dependency injection - Phase 2: Provide context for refactored code
    GameContext get_context() noexcept;

    // ... rest of class ...
};

// extern Game game;  // Global instance at bottom of file
```

---

## Pickable Base Class (Relevant Excerpt from src/Actor/Pickable.h)
```cpp
class Pickable : public Persistent
{
public:
    enum class PickableType : int
    {
        HEALER,
        LIGHTNING_BOLT,
        CONFUSER,
        // ... other types ...
    };

    virtual ~Pickable() {};

    virtual bool use(Item& owner, Creature& wearer);
    static std::unique_ptr<Pickable> create(const json& j);
    virtual void save(json& j) = 0;
    virtual void load(const json& j) = 0;
    virtual PickableType get_type() const = 0;
};
```

---

## Call Site: InventoryUI.cpp (src/UI/InventoryUI.cpp:317)
```cpp
bool InventoryUI::handle_backpack_selection(Player& player, int itemIndex)
{
    if (is_inventory_empty(player.inventory_data))
    {
        game.message(WHITE_BLACK_PAIR, "Your backpack is empty.", true);
        return true;
    }

    // Build list of valid items (skipping null entries)
    std::vector<Item*> validItems;
    for (const auto& item : player.inventory_data.items)
    {
        if (item)
        {
            validItems.push_back(item.get());
        }
    }

    // ... debug logging ...

    if (itemIndex >= 0 && itemIndex < static_cast<int>(validItems.size()))
    {
        Item* selectedItem = validItems[itemIndex];

        // ... more logging ...

        if (selectedItem && selectedItem->pickable)
        {
            Item* itemPtr = selectedItem;

            game.log("Attempting to use item...");
            bool itemUsed = selectedItem->pickable->use(*selectedItem, player);  // <<< LINE 317
            game.log("Item use result: " + std::string(itemUsed ? "SUCCESS" : "FAILED"));

            if (itemUsed)
            {
                game.gameStatus = Game::GameStatus::NEW_TURN;

                // Check if the item still exists in inventory (not consumed)
                bool itemStillExists = false;
                for (const auto& item : player.inventory_data.items)
                {
                    if (item.get() == itemPtr)
                    {
                        itemStillExists = true;
                        break;
                    }
                }

                if (!itemStillExists)
                {
                    return false; // Exit inventory
                }
            }

            return true;
        }
        else
        {
            game.log("Item has no pickable component!");
        }
    }
    else
    {
        game.log("Index out of bounds!");
    }

    game.message(WHITE_BLACK_PAIR, "Invalid backpack selection.", true);
    return true;
}
```

---

## Colors.h References (Used in Healer)
```cpp
// From src/Colors/Colors.h (implied usage)
#define COLOR_WHITE  (number)   // Used in Healer for "You heal" and "hit points"
#define COLOR_RED    (number)   // Used in Healer for damage amount and failure message
#define WHITE_BLACK_PAIR  (number)
#define WHITE_RED_PAIR    (number)
```

---

## MessageSystem.h (Relevant Methods)
```cpp
class MessageSystem
{
public:
    // Core message functionality
    void message(int color, std::string_view text, bool isComplete = false);
    void append_message_part(int color, std::string_view text);
    void finalize_message();
    void transfer_messages_to_gui(Gui& gui);

    // Debug logging
    void log(std::string_view message) const;
    void display_debug_messages() const noexcept;

    // Debug mode control
    void enable_debug_mode() noexcept { debugMode = true; }
    void disable_debug_mode() noexcept { debugMode = false; }
    bool is_debug_mode() const noexcept { return debugMode; }

    // ... other methods ...
};
```

---

## PROPOSED CODE CHANGES

### Proposed Pickable.h Addition
```cpp
// Forward declaration (add at top after existing includes)
struct GameContext;

// In class Pickable (add new virtual method)
class Pickable : public Persistent
{
public:
    // ... existing methods ...
    virtual bool use(Item& owner, Creature& wearer);

    // NEW METHOD - Optional override for dependency injection
    virtual bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
    {
        // Default implementation: delegate to old use() method
        return use(owner, wearer);
    }

    // ... rest of class ...
};
```

### Proposed Healer.h Addition
```cpp
#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

// Forward declaration
struct GameContext;

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 }; // how many hp

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer) override;
	// NEW: Dependency-injected version
	bool use_with_context(Item& owner, Creature& wearer, GameContext& ctx) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;
};
//====
```

### Proposed Healer.cpp Implementation (NEW METHOD)
```cpp
#include "Healer.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"

//==HEALER==

Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

// EXISTING METHOD - Keep for backward compatibility
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

// NEW METHOD - Dependency-injected version
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

void Healer::load(const json& j)
{
	if (j.contains("amountToHeal") && j["amountToHeal"].is_number())
	{
		amountToHeal = j["amountToHeal"].get<int>();
	}
	else
	{
		throw std::runtime_error("Invalid JSON format for Healer");
	}
}

void Healer::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::HEALER);
	j["amountToHeal"] = amountToHeal;
}

Pickable::PickableType Healer::get_type() const
{
	return PickableType::HEALER;
}
```

### Proposed InventoryUI.cpp Change (Line 317)
```cpp
// CURRENT CODE (Line 317):
bool itemUsed = selectedItem->pickable->use(*selectedItem, player);

// PROPOSED CODE:
auto ctx = game.get_context();
bool itemUsed = selectedItem->pickable->use_with_context(*selectedItem, player, ctx);
```

---

## Test Template for Refactored Code
```cpp
// Example test setup for Healer::use_with_context()

#include <gtest/gtest.h>
#include "Healer.h"
#include "GameContext.h"
#include "MessageSystem.h"

class HealerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test context
        test_message_system = std::make_unique<MessageSystem>();
        test_ctx.message_system = test_message_system.get();

        // Initialize other context members as needed
        // ...

        healer = std::make_unique<Healer>(20);  // 20 HP heal
    }

    GameContext test_ctx;
    std::unique_ptr<MessageSystem> test_message_system;
    std::unique_ptr<Healer> healer;
};

TEST_F(HealerTest, UseWithContextHealsCreature)
{
    // Setup
    Creature creature{/* ... */};
    Item item{/* ... */};

    // Test
    bool result = healer->use_with_context(item, creature, test_ctx);

    // Verify
    EXPECT_TRUE(result);
    // Verify message_system was called
}

TEST_F(HealerTest, UseWithContextHandlesMaxHealth)
{
    // Setup
    Creature creature_at_max_health{/* ... */};
    Item item{/* ... */};

    // Test
    bool result = healer->use_with_context(item, creature_at_max_health, test_ctx);

    // Verify
    EXPECT_FALSE(result);
    // Verify "Health is already maxed out!" message
}
```

---

## Execution Flow Comparison

### BEFORE Refactoring (Current)
```
InventoryUI::handle_backpack_selection()
  ├─ selectedItem = validItems[itemIndex]
  ├─ selectedItem->pickable->use(*selectedItem, player)
  │   └─ Healer::use(Item& owner, Creature& wearer)
  │       ├─ wearer.destructible->heal(amountToHeal)
  │       ├─ if (amountHealed > 0)
  │       │   ├─ game.message(COLOR_WHITE, "You heal ", false)
  │       │   │   └─ game.message_system.message(...)
  │       │   ├─ game.message(COLOR_RED, std::to_string(...), false)
  │       │   │   └─ game.message_system.message(...)
  │       │   ├─ game.message(COLOR_WHITE, " hit points.", true)
  │       │   │   └─ game.message_system.message(...)
  │       │   └─ return Pickable::use(owner, wearer)
  │       └─ else
  │           └─ game.message(COLOR_RED, "Health is already maxed out!", true)
  │               └─ game.message_system.message(...)
  ├─ itemUsed = result
  └─ Update game state and UI
```

### AFTER Refactoring (Proposed)
```
InventoryUI::handle_backpack_selection()
  ├─ selectedItem = validItems[itemIndex]
  ├─ ctx = game.get_context()
  ├─ selectedItem->pickable->use_with_context(*selectedItem, player, ctx)
  │   └─ Healer::use_with_context(Item& owner, Creature& wearer, GameContext& ctx)
  │       ├─ wearer.destructible->heal(amountToHeal)
  │       ├─ if (amountHealed > 0)
  │       │   ├─ ctx.message_system->message(COLOR_WHITE, "You heal ", false)
  │       │   ├─ ctx.message_system->message(COLOR_RED, std::to_string(...), false)
  │       │   ├─ ctx.message_system->message(COLOR_WHITE, " hit points.", true)
  │       │   └─ return Pickable::use(owner, wearer)
  │       └─ else
  │           └─ ctx.message_system->message(COLOR_RED, "Health is already maxed out!", true)
  ├─ itemUsed = result
  └─ Update game state and UI
```

---

## Dependency Mapping

### Current Dependencies (Global)
```
Healer
  └─ extern Game game
     └─ Game::message()
        └─ Game::message_system (member)
           └─ MessageSystem::message()
```

### Proposed Dependencies (Injected)
```
Healer
  └─ GameContext& ctx (parameter)
     └─ ctx.message_system (pointer member)
        └─ MessageSystem::message()
```

---

## Compilation Checks

### Header Dependencies After Refactoring

**Healer.h will need**:
```cpp
#include "../Actor/Pickable.h"  // Already has
// New:
struct GameContext;  // Forward declaration only
```

**Healer.cpp will need**:
```cpp
#include "Healer.h"
#include "../Game.h"
#include "../Colors/Colors.h"
// New:
#include "../Core/GameContext.h"  // For actual use
```

**Pickable.h will need**:
```cpp
// New:
struct GameContext;  // Forward declaration
```

### No New External Dependencies Required
- MessageSystem already defined in GameContext
- Colors constants already accessible
- All classes already in use

---

## Document References

- Main analysis: REFACTORING_ANALYSIS_HEALER.md
- Summary: HEALER_REFACTORING_SUMMARY.txt
- Detailed refs: HEALER_DETAILED_REFERENCES.csv
- This document: HEALER_CODE_REFERENCE.md

