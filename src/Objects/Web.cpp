#include "Web.h"
#include "../Game.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"

Web::Web(Vector2D position, int strength)
    : Object(position, ActorData{ '*', "spider web", BLACK_WHITE_PAIR }),
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

    // Calculate chance to get caught based on dexterity and web strength
    int catchChance = 40 + (webStrength * 10) - ((ctx.player->get_dexterity() - 10) * 3);
    catchChance = std::min(90, std::max(10, catchChance));  // Cap between 10-90%

    if (ctx.dice_roller->d100() <= catchChance)
    {
        // Calculate number of turns stuck based on web strength
        int stuckTurns = webStrength + ctx.dice_roller->roll(1, 2);

        // Apply the effect
        ctx.player->get_stuck_in_web(stuckTurns, webStrength, this, ctx);

        ctx.message_system->message(WHITE_BLACK_PAIR, "You're caught in a sticky web!", true);

        // Player loses their turn
        *ctx.game_status = static_cast<int>(Game::GameStatus::NEW_TURN);
        return true;
    }
    else
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "You carefully navigate through the web.", true);

        // 50% chance to destroy the web
        if (ctx.dice_roller->d2() == 1)
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

    // For now, just remove it from the game's object list
    auto it = std::find_if(ctx.objects->begin(), ctx.objects->end(),
        [this](const auto& obj) { return obj.get() == this; });

    if (it != ctx.objects->end()) {
        it->reset();
    }
}