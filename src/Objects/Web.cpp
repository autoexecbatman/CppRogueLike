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
bool Web::applyEffect(Creature& creature)
{
    // Only players can get stuck (for simplicity)
    if (&creature != game.player.get()) return false;

    // Calculate chance to get caught based on dexterity and web strength
    int catchChance = 40 + (webStrength * 10) - ((game.player->get_dexterity() - 10) * 3);
    catchChance = std::min(90, std::max(10, catchChance));  // Cap between 10-90%

    if (game.d.d100() <= catchChance)
    {
        // Calculate number of turns stuck based on web strength
        int stuckTurns = webStrength + game.d.roll(1, 2);

        // Apply the effect
        game.player->get_stuck_in_web(stuckTurns, webStrength, this);

        game.message(WHITE_BLACK_PAIR, "You're caught in a sticky web!", true);

        // Player loses their turn
        game.gameStatus = Game::GameStatus::NEW_TURN;
        return true;
    }
    else
    {
        game.message(WHITE_BLACK_PAIR, "You carefully navigate through the web.", true);

        // 50% chance to destroy the web
        if (game.d.d2() == 1)
        {
            destroy();
            game.message(WHITE_BLACK_PAIR, "You tear through the web, clearing a path.", true);
        }

        return false;
    }
}

// Destroy this web
void Web::destroy()
{
    // Mark for deletion
    // We don't delete it here directly because it could be mid-update
    // Instead we set a flag or use a system that safely removes objects

    // For now, just remove it from the game's object list
    auto it = std::find_if(game.objects.begin(), game.objects.end(),
        [this](const auto& obj) { return obj.get() == this; });

    if (it != game.objects.end()) {
        it->reset();
    }
}