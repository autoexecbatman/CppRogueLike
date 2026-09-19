#pragma once

#include <memory>
#include <string_view>

#include "ItemEnhancements.h"
#include "ItemRegistry.h"

// file: ItemCreator.h
//
// Builds items from their parameters: gives each the behaviour its type names, its
// value, its carry weight and its tile. The parameters come from ctx.itemRegistry and
// the tile from ctx.contentRegistry.
//
// Usage:
//
//   auto sword = ItemCreator::create("long_sword", position, ctx); // throws std::out_of_range for an unknown key
//   auto pile = ItemCreator::create_with_gold_amount(position, 40, ctx); // worth 40
//   auto potion = ItemCreator::create_random_of_category("potion", position, ctx, 1); // nullptr if none can appear

class Item;
struct Vector2D;
struct GameContext;

namespace ItemCreator
{
// The item a key names, at pos. Throws std::out_of_range if the key is unknown.
[[nodiscard]] std::unique_ptr<Item> create(std::string_view key, Vector2D pos, GameContext& ctx);

// A pile of gold at pos, paying and worth goldAmount. Throws if the registry has no
// gold_coin.
[[nodiscard]] std::unique_ptr<Item> create_with_gold_amount(Vector2D pos, int goldAmount, GameContext& ctx);

// The item a key names, at pos, with an enhancement's prefix and suffix applied.
[[nodiscard]] std::unique_ptr<Item> create_with_enhancement(std::string_view key, Vector2D pos, PrefixType prefix, SuffixType suffix, GameContext& ctx);

// A pile of 5 to 20 gold at pos.
[[nodiscard]] std::unique_ptr<Item> create_gold_pile(Vector2D pos, GameContext& ctx);

// One item of a category drawn by spawn weight at dungeonLevel, or nullptr when none of
// the category can appear there.
[[nodiscard]] std::unique_ptr<Item> create_random_of_category(std::string_view category, Vector2D pos, GameContext& ctx, int dungeonLevel);
} // namespace ItemCreator
