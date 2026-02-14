#include "Web.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"

Web::Web(Vector2D position, int strength)
    : Object(position, ActorData{ TILE_WEB, "spider web", BLACK_WHITE_PAIR }),
    webStrength(strength)
{
    // Webs don't block movement but do have their effect when passed through
    // Note that webs don't have the BLOCKS state
}

// Apply web effect when a creature tries to pass through
bool Web::applyEffect(Creature& creature, GameContext& ctx)
{
    // Only players can get stuck (for simplicity)
    if (&creature != ctx.player) return false;

    // Check for Ring of Free Action (AD&D 2e: grants immunity to webs and paralysis)
    for (const auto slot : {EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING})
    {
        if (Item* equippedRing = creature.get_equipped_item(slot))
        {
            if (const auto* magicRing = dynamic_cast<const MagicalRing*>(equippedRing->pickable.get()))
            {
                if (magicRing->effect == MagicalEffect::FREE_ACTION)
                {
                    ctx.message_system->message(CYAN_BLACK_PAIR, "Your ring of free action protects you from the web!", true);
                    destroy(ctx);
                    return false;
                }
            }
        }
    }

    // Calculate chance to get caught based on dexterity and web strength
    int catchChance = 40 + (webStrength * 10) - ((ctx.player->get_dexterity() - 10) * 3);
    catchChance = std::min(90, std::max(10, catchChance));  // Cap between 10-90%

    if (ctx.dice->d100() <= catchChance)
    {
        // Calculate number of turns stuck based on web strength
        int stuckTurns = webStrength + ctx.dice->roll(1, 2);

        // Apply the effect
        ctx.player->get_stuck_in_web(stuckTurns, webStrength, this, ctx);

        ctx.message_system->message(WHITE_BLACK_PAIR, "You're caught in a sticky web!", true);

        // Player loses their turn
        *ctx.game_status = GameStatus::NEW_TURN;
        return true;
    }
    else
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "You carefully navigate through the web.", true);

        // 50% chance to destroy the web
        if (ctx.dice->d2() == 1)
        {
            destroy(ctx);
            ctx.message_system->message(WHITE_BLACK_PAIR, "You tear through the web, clearing a path.", true);
        }

        return false;
    }
}

// Destroy this web
void Web::destroy(GameContext& ctx)
{
    // Mark for deletion
    // We don't delete it here directly because it could be mid-update
    // Instead we set a flag or use a system that safely removes objects

    // C++20 ranges: find and reset the web object
    auto found = std::ranges::find_if(*ctx.objects,
        [this](const auto& obj) { return obj.get() == this; });

    if (found != ctx.objects->end()) {
        found->reset();
    }
}