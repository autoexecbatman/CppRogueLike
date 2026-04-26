// file: CurseSystem.cpp
#include "CurseSystem.h"

#include <format>

#include "../Actor/Destructible.h"
#include "../Actor/Item.h"
#include "../ActorTypes/Player.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "../Items/ItemIdentification.h"
#include "../Systems/MessageSystem.h"

// Emits "weakens your aim" notification for cursed weapons.
void CurseSystem::apply_weapon_curse(const Item& item, GameContext& ctx)
{
	if (!ctx.messageSystem)
	{
		return;
	}
	ctx.messageSystem->message(
		14,
		std::format("Your {} weakens your aim!", item.actorData.name),
		true);
}

// Emits "deteriorates" notification for cursed armor.
void CurseSystem::apply_armor_curse(const Item& item, GameContext& ctx)
{
	if (!ctx.messageSystem)
	{
		return;
	}
	ctx.messageSystem->message(
		14,
		std::format("Your {} deteriorates under the curse!", item.actorData.name),
		true);
}

// Drains 1 HP per turn and emits "drains" notification for cursed amulets.
void CurseSystem::apply_hp_drain(int damage, Player& player, GameContext& ctx)
{
	if (damage <= 0)
	{
		return;
	}

	const int damageTaken = player.take_damage(damage, ctx, DamageType::MAGIC);
	if (player.is_dead())
	{
		player.die(ctx);
	}

	if (ctx.messageSystem)
	{
		ctx.messageSystem->message(
			15,
			std::format("The curse drains {} HP from you!", damageTaken),
			true);
	}

	if (player.get_hp() <= 0)
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(12, "The curse has killed you!", true);
		}
		ctx.gameState->set_game_status(GameStatus::DEFEAT);
	}
}

// Called once per NEW_TURN from GameLoopCoordinator::update().
void CurseSystem::apply_curses(Player& player, GameContext& ctx)
{
    for (const auto& equipped : player.equippedItems)
    {
        if (!equipped.item)
        {
            continue;
        }

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
