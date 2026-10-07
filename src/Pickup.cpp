#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "ListMenu.h"
#include "MenuEntry.h"
#include "MessageSystem.h"
#include "Pickable.h"
#include "Pickup.h"
#include "UniqueId.h"

namespace
{
// Entries are lettered from here, so the first reads 'a'. Twenty-six is past any heap
// a tile can hold, the floor inventory being smaller than that.
constexpr char FIRST_HOTKEY = 'a';
} // namespace

namespace Pickup
{

void take_floor_item(Creature& taker, Item& item, GameContext& ctx)
{
	assert(ctx.floorInventory && "take_floor_item: no floor to take from");

	// Coin never enters the pack. The purse carries it, and a purse's weight is
	// answered from the gold total rather than from an item nobody is holding.
	if (item.itemClass == ItemClass::GOLD_COIN)
	{
		Gold& goldBehavior = std::get<Gold>(*item.behavior);
		taker.adjust_gold(goldBehavior.amount);
		ctx.messageSystem->message(
			ColorPairId::YELLOW_BLACK,
			std::format("You picked up {} gold.", goldBehavior.amount),
			MessageCompletion::FINISHED);
		[[maybe_unused]] const auto takeGoldResult = InventoryOperations::remove_item(*ctx.floorInventory, item);
		assert(takeGoldResult.has_value());
		return;
	}

	// Slot capacity, checked before ownership moves.
	if (InventoryOperations::is_inventory_full(taker.inventoryData))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your inventory is full!", MessageCompletion::FINISHED);
		return;
	}

	// Weight, likewise: a refusal after the floor had let go would destroy the item.
	if (!InventoryOperations::is_within_weight_limit(item, taker, *ctx.dataManager))
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "Too heavy to carry.", MessageCompletion::FINISHED);
		return;
	}

	// Read before the move: the item is gone from under this reference afterwards.
	const std::string itemName = item.actorData.name;

	auto removeResult = InventoryOperations::remove_item(*ctx.floorInventory, item);
	if (!removeResult.has_value())
	{
		ctx.messageSystem->log("ERROR: take_floor_item -- remove_item failed unexpectedly");
		return;
	}

	// The pre-checks passed, so this add cannot fail on capacity or weight.
	auto addResult = InventoryOperations::add_item_to_inventory(
		taker.inventoryData,
		std::move(*removeResult),
		taker,
		*ctx.dataManager);

	if (addResult.has_value())
	{
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("You picked up the {}.", itemName),
			MessageCompletion::FINISHED);
	}
	else
	{
		ctx.messageSystem->log("ERROR: take_floor_item -- add failed after pre-checks; item lost");
	}
}

void from_floor(Creature& taker, GameContext& ctx)
{
	assert(ctx.floorInventory && "from_floor: no floor to take from");

	const std::vector<Item*> standing = InventoryOperations::items_at(*ctx.floorInventory, taker.position);

	if (standing.empty())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There's nothing here to pick up.", MessageCompletion::FINISHED);
		return;
	}

	// One item is no choice at all, so taking it costs the keypress already spent.
	if (standing.size() == 1)
	{
		take_floor_item(taker, *standing.front(), ctx);
		return;
	}

	assert(ctx.menus && "from_floor: a heap needs a menu and there is nowhere to put one");

	std::vector<MenuEntry> entries{};
	char hotkey = FIRST_HOTKEY;

	for (Item* item : standing)
	{
		assert(item && "from_floor: the floor holds a null where an item should be");

		// Init-capture by id: this menu outlives the frame that built it, and the
		// floor can lose an item in between, so the choice is resolved against the
		// floor rather than against a pointer taken now.
		auto takeCommand = [who = &taker, wanted = item->uniqueId](GameContext& menuCtx)
		{
			Item* stillThere = InventoryOperations::find_item_by_id(*menuCtx.floorInventory, wanted);
			if (stillThere == nullptr)
			{
				menuCtx.messageSystem->message(ColorPairId::WHITE_BLACK, "It is not there any more.", MessageCompletion::FINISHED);
			}
			else
			{
				take_floor_item(*who, *stillThere, menuCtx);
			}
			menuCtx.menus->back()->back = true;
		};

		entries.push_back({ item->actorData.name, hotkey, takeCommand });
		++hotkey;
	}

	ctx.menus->push_back(std::make_unique<ListMenu>(
		"Pick up what?",
		std::move(entries),
		std::function<void(GameContext&)>{},
		std::function<void(GameContext&)>{},
		ctx));
}

} // namespace Pickup
