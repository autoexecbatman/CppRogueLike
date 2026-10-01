// file: CurseSystem.cpp
#include "CurseSystem.h"
#include <cassert>

#include <format>

#include "DamageInfo.h"
#include "GameContext.h"
#include "Item.h"
#include "ItemIdentification.h"
#include "MessageSystem.h"
#include "Player.h"

// Emits "weakens your aim" notification for cursed weapons.
void CurseSystem::apply_weapon_curse(const Item& item, GameContext& ctx)
{
	assert(ctx.messageSystem && "apply_weapon_curse: no message system to tell the wearer with");

	ctx.messageSystem->message(
		ColorPairId::MAGENTA_BLACK,
		std::format("Your {} weakens your aim!", item.actorData.name),
		MessageCompletion::FINISHED);
}

// Emits "deteriorates" notification for cursed armor.
void CurseSystem::apply_armor_curse(const Item& item, GameContext& ctx)
{
	assert(ctx.messageSystem && "apply_armor_curse: no message system to tell the wearer with");

	ctx.messageSystem->message(
		ColorPairId::MAGENTA_BLACK,
		std::format("Your {} deteriorates under the curse!", item.actorData.name),
		MessageCompletion::FINISHED);
}

// Drains 1 HP per turn and emits "drains" notification for cursed amulets.
void CurseSystem::apply_hp_drain(int damage, Player& player, GameContext& ctx)
{
	if (damage <= 0)
	{
		return;
	}

	const int damageTaken = player.take_damage(damage, ctx, DamageType::MAGIC);
	if (ctx.messageSystem)
	{
		ctx.messageSystem->message(
			ColorPairId::CYAN_BLUE,
			std::format("The curse drains {} HP from you!", damageTaken),
			MessageCompletion::FINISHED);
	}

	// The drain is reported before the death; die owns the defeat status.
	if (player.is_dead())
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(ColorPairId::BLUE_BLACK, "The curse has killed you!", MessageCompletion::FINISHED);
		}
		player.die(ctx);
	}
}

// Called once per NEW_TURN from GameLoopCoordinator::update().
void CurseSystem::apply_curses(Player& player, GameContext& ctx)
{
	for (const auto& equipped : player.equippedItems)
	{
		const Item& item = *equipped.item;

		if (item.get_enhancement().blessing != BlessingStatus::CURSED)
		{
			continue;
		}

		if (item.is_weapon())
		{
			apply_weapon_curse(item, ctx);
		}
		else if (item.is_armor())
		{
			apply_armor_curse(item, ctx);
		}
		else if (item.is_amulet())
		{
			apply_hp_drain(1, player, ctx);
		}
	}
}
