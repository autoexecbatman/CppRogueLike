#pragma once

// file: Pickup.h
//
// Taking things off the dungeon floor.
//
// Two entry points. from_floor is the player action: it looks at what is underfoot
// and either hands over the one item there or asks which one. take_floor_item is the
// single transfer, and is what each entry of that menu ends up calling.
//
// Usage:
//
//   Pickup::from_floor(player, ctx);        // the whole action, menu and all
//   Pickup::take_floor_item(player, rope, ctx);  // one named item, no asking

class Creature;
class Item;
struct GameContext;

namespace Pickup
{

// Takes one named floor item: the purse for coin, the pack for everything else.
// Reports what happened to the message log itself, so a caller has nothing left to
// decide. Refuses, with a message and the item left where it lies, when the pack is
// full or the item would take the taker past its weight limit.
//
// Example:
//   take_floor_item(player, rope, ctx);      // "You picked up the rope."
//   take_floor_item(player, goldPile, ctx);  // "You picked up 40 gold." - the purse
//   take_floor_item(weakling, anvil, ctx);   // "Too heavy to carry." - still on the floor
void take_floor_item(Creature& taker, Item& item, GameContext& ctx);

// Takes whatever the taker is standing on. A bare tile says so, one item is handed
// over at once, and a heap opens a menu of what is there and takes nothing until the
// choice lands.
//
// The menu outlives the frame that built it, so its entries hold item ids rather than
// pointers and ask the floor again when chosen. An item that is gone by then says so
// instead of being followed into freed memory.
//
// Example:
//   from_floor(player, ctx);   // bare tile  -> "There's nothing here to pick up."
//   from_floor(player, ctx);   // one dagger -> "You picked up the dagger."
//   from_floor(player, ctx);   // three items -> a menu titled "Pick up what?"
void from_floor(Creature& taker, GameContext& ctx);

} // namespace Pickup
