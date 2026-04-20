// file: CurseSystem.cpp
#include "CurseSystem.h"

#include <format>
#include <string>

#include "../Actor/Destructible.h"
#include "../Actor/Item.h"
#include "../ActorTypes/Player.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "../Items/ItemIdentification.h"
#include "../Systems/MessageSystem.h"

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

		// AD&D 2e: cursed amulets drain 1 HP per turn.
		// All other curse effects are enforced at their computation sites — see header.
		if (item.is_amulet())
		{
			apply_hp_drain(1, player, ctx);
		}
	}
}

void CurseSystem::apply_hp_drain(int damage, Player& player, GameContext& ctx)
{
	if (damage <= 0)
	{
		return;
	}

	const int damageTaken = player.destructible->take_damage(player, damage, ctx, DamageType::MAGIC);

	if (ctx.messageSystem)
	{
		ctx.messageSystem->message(
			15,
			std::format("The curse drains {} HP from you!", damageTaken),
			true);
	}

	if (player.destructible->get_hp() <= 0)
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(12, "The curse has killed you!", true);
		}
		ctx.gameState->set_game_status(GameStatus::DEFEAT);
	}
}
