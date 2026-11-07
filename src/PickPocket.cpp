// file: PickPocket.cpp
// The thief's Pick Pockets skill; PickPocket.h holds the rule and its usage.

#include "PickPocket.h"

#include <cassert>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Attitude.h"
#include "BaseMenu.h"
#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "ListMenu.h"
#include "MenuEntry.h"
#include "MessageSystem.h"
#include "Player.h"
#include "PlayerTurn.h"
#include "RandomDice.h"
#include "ShopKeeper.h"
#include "ThiefSkills.h"
#include "TurnSchedule.h"

namespace CppRL
{
	namespace
	{
		// What a creature carries for a thief to take: a shopkeeper's
		// shelves, anyone else's pack.
		//
		// Example:
		//   carried_goods(shopkeeper);   // -> its shop stock
		//   carried_goods(orc);          // -> orc.inventoryData
		CreatureInventory& carried_goods(Creature& holder) noexcept
		{
			if (holder.shop)
			{
				return holder.shop->get_shop_inventory();
			}
			return holder.inventoryData;
		}

		// The thief's usable Pick Pockets score, or nothing for a class
		// without the skill or a score the adjustments leave at zero.
		//
		// Example:
		//   usable_score(rogueWithThirtyBought);   // -> 50
		//   usable_score(fighter);                 // -> std::nullopt
		std::optional<int> usable_score(const Player& thief)
		{
			const std::optional<int> score =
				thief.thief_skill(ThiefSkill::PICK_POCKETS);
			if (!score.has_value() || score.value() <= 0)
			{
				return std::nullopt;
			}
			return score;
		}

		void say_cannot_pick_pockets(GameContext& ctx)
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				"You cannot pick pockets.",
				MessageCompletion::FINISHED);
		}

		// One try at a chosen victim, and the round it costs. A refusal is
		// free, so only a try is charged.
		//
		// Example:
		//   try_and_spend(shopkeeper, ctx);   // a try: the clock moves 120
		void try_and_spend(Creature& victim, GameContext& ctx)
		{
			const PickPocketAttempt attempt =
				attempt_pick_pocket(ctx.player_concrete(), victim, ctx);
			if (attempt == PickPocketAttempt::TOOK_NOTHING ||
				attempt == PickPocketAttempt::TOOK_ITEM)
			{
				spend_player_action(ctx, TIME_UNITS_PER_ROUND);
			}
		}

		// Asks which of several victims to try, one lettered line each.
		//
		// Example, a shopkeeper and an orc beside the thief:
		//   ask_which_victim({ &shopkeeper, &orc }, ctx);
		//   // a menu titled "Pick whose pocket?": shopkeeper on a, orc on b
		void ask_which_victim(
			const std::vector<Creature*>& victims,
			GameContext& ctx)
		{
			std::vector<MenuEntry> entries{};
			char hotkey = 'a';
			for (Creature* victim : victims)
			{
				assert(victim && "ask_which_victim: a null victim was offered");

				// Init-capture: the menu outlives this call, so the victim is
				// held by pointer; nothing dies while the menu is open.
				auto robVictim = [chosen = victim](GameContext& menuCtx)
				{
					menuCtx.menus->back()->back = true;
					try_and_spend(*chosen, menuCtx);
				};
				entries.push_back(
					{ victim->actorData.name, hotkey, robVictim });
				++hotkey;
			}

			ctx.menus->push_back(
				std::make_unique<ListMenu>(
					"Pick whose pocket?",
					std::move(entries),
					std::function<void(GameContext&)>{},
					std::function<void(GameContext&)>{},
					ctx));
		}
	} // namespace

	int pick_pocket_notice_roll(int victimLevel) noexcept
	{
		return 100 - 3 * victimLevel;
	}

	std::vector<Creature*> pick_pocket_victims(
		const Creature& thief,
		const std::vector<std::unique_ptr<Creature>>& creatures)
	{
		std::vector<Creature*> victims{};
		for (const auto& creature : creatures)
		{
			assert(
				creature &&
				"pick_pocket_victims: a null creature is in the list");

			// Only a living neighbour with something on it can be robbed.
			const bool isBeside = creature.get() != &thief &&
				thief.get_tile_distance(creature->position) == 1;
			if (isBeside && !creature->is_dead() &&
				!carried_goods(*creature).items.empty())
			{
				victims.push_back(creature.get());
			}
		}
		return victims;
	}

	PickPocketAttempt attempt_pick_pocket(
		Player& thief,
		Creature& victim,
		GameContext& ctx)
	{
		assert(ctx.dice && "attempt_pick_pocket: called without dice");
		assert(
			ctx.messageSystem &&
			"attempt_pick_pocket: called without a messageSystem");
		assert(
			ctx.dataManager &&
			"attempt_pick_pocket: called without a dataManager");

		const std::optional<int> score = usable_score(thief);
		if (!score.has_value())
		{
			say_cannot_pick_pockets(ctx);
			return PickPocketAttempt::NOT_A_THIEF;
		}

		if (InventoryOperations::is_inventory_full(thief.inventoryData))
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_RED,
				"Your pack is full.",
				MessageCompletion::FINISHED);
			return PickPocketAttempt::PACK_FULL;
		}

		CreatureInventory& goods = carried_goods(victim);
		assert(
			!goods.items.empty() &&
			"attempt_pick_pocket: the victim carries nothing to take");

		// One roll answers both questions, so they can be true together.
		const int roll = ctx.dice->d100();
		const bool isNoticed =
			roll >= pick_pocket_notice_roll(victim.get_creature_level());

		PickPocketAttempt attempt = PickPocketAttempt::TOOK_NOTHING;
		if (roll <= score.value())
		{
			// The thief takes what his fingers find, not what he would choose.
			const size_t index = static_cast<size_t>(
				ctx.dice->roll(0, static_cast<int>(goods.items.size()) - 1));
			const Item& chosen = *goods.items.at(index);
			if (InventoryOperations::is_within_weight_limit(
					chosen,
					thief,
					*ctx.dataManager))
			{
				const std::string itemName = chosen.get_name();
				auto taken = InventoryOperations::remove_item_at(goods, index);
				assert(
					taken.has_value() &&
					"attempt_pick_pocket: an item at a valid index was not removed");

				// Room and weight were checked, so the pack takes it.
				[[maybe_unused]] const auto stowed =
					InventoryOperations::add_item_to_inventory(
						thief.inventoryData,
						std::move(*taken),
						thief,
						*ctx.dataManager);
				assert(
					stowed.has_value() &&
					"attempt_pick_pocket: the pack refused an item checked to fit");

				ctx.messageSystem->message(
					ColorPairId::CYAN_BLACK,
					std::format(
						"You lift the {} from the {}.",
						itemName,
						victim.actorData.name),
					MessageCompletion::FINISHED);
				attempt = PickPocketAttempt::TOOK_ITEM;
			}
			else
			{
				ctx.messageSystem->message(
					ColorPairId::WHITE_BLACK,
					std::format(
						"The {} is too heavy to lift.",
						chosen.get_name()),
					MessageCompletion::FINISHED);
			}
		}
		else
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				"Your fingers come away empty.",
				MessageCompletion::FINISHED);
		}

		// Being caught is what ends the tries: the victim turns on the thief.
		if (isNoticed)
		{
			victim.set_attitude(Attitude::HOSTILE);
			ctx.messageSystem->message(
				ColorPairId::RED_BLACK,
				std::format("The {} catches you!", victim.actorData.name),
				MessageCompletion::FINISHED);
		}
		return attempt;
	}

	void pick_pocket(GameContext& ctx)
	{
		assert(ctx.creatures && "pick_pocket: called without creatures");
		assert(ctx.menus && "pick_pocket: called without menus");

		// A player without the skill hears so before anyone is looked for.
		Player& thief = ctx.player_concrete();
		if (!usable_score(thief).has_value())
		{
			say_cannot_pick_pockets(ctx);
			return;
		}

		const std::vector<Creature*> victims =
			pick_pocket_victims(thief, *ctx.creatures);
		if (victims.empty())
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				"There is nothing within reach to steal.",
				MessageCompletion::FINISHED);
			return;
		}

		// One victim needs no question; several are the player's choice.
		if (victims.size() == 1)
		{
			try_and_spend(*victims.front(), ctx);
			return;
		}
		ask_which_victim(victims, ctx);
	}

} // namespace CppRL
