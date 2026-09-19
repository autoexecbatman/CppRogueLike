#pragma once

#include <string>
#include <vector>

// file: ItemFactory.h
//
// Draws random items for a dungeon level, onto the floor. The table it draws from is
// derived from an ItemRegistry on every call - the registry's items with a spawn weight,
// in key order, then its enhanced spawn rules - so nothing keeps a copy that an edit, a
// new level or a game loaded from a save could find stale.
//
// Usage:
//
//   ItemFactory::spawn_random_item(position, dungeonLevel, ctx); // one item onto ctx.floorInventory
//   ItemFactory::spawn_item_of_category(position, dungeonLevel, "potion", ctx); // one potion, if any can appear
//   ItemFactory::generate_treasure(position, dungeonLevel, 2, ctx); // gold and two or three items
//   ItemFactory::get_current_distribution(1, *ctx.itemRegistry); // -> one { name, category, percentage } per entry

struct Vector2D;
struct GameContext;
class ItemRegistry;

// One table entry's chance of being drawn, in percent.
struct ItemPercentage
{
	// The item's name, or an enhanced rule's category.
	std::string name{};
	// The spawn category it is drawn under.
	std::string category{};
	// Its share of the level's total spawn weight, 0 to 100.
	float percentage{};
};

namespace ItemFactory
{
// Places a gold pile worth 10 to 20 gold per dungeon level per quality at position,
// then one to five items near it, more and deeper-level ones for higher quality (1 to 3).
void generate_treasure(Vector2D position, int dungeonLevel, int quality, GameContext& ctx);

// Each entry's chance at dungeonLevel, from the registry as it stands; an entry that
// cannot appear there is left out.
[[nodiscard]] std::vector<ItemPercentage> get_current_distribution(int dungeonLevel, const ItemRegistry& items);

// Places one item of category at position, drawn by weight from ctx.itemRegistry for
// dungeonLevel. Logs and places nothing when none of the category can appear.
void spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category, GameContext& ctx);

// Places one item of any category at position, drawn by weight from ctx.itemRegistry
// for dungeonLevel. Logs and places nothing when no item can appear.
void spawn_random_item(Vector2D position, int dungeonLevel, GameContext& ctx);

// Debug: places one enhanced item from every enhanced spawn rule at position.
void spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx);
} // namespace ItemFactory
