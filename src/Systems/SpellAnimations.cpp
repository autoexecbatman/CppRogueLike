#include "SpellAnimations.h"

#include <cmath>
#include <memory>
#include <vector>

#include "../Core/GameContext.h"
#include "../Utils/Vector2D.h"
#include "../Colors/Colors.h"
#include "../Systems/RenderingManager.h"
#include "../Gui/Gui.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"

namespace SpellAnimations
{
    void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
    {
        // TODO: stub - lightning animation requires renderer replacement
        // Previously drew jagged bolt path with COLOR_PAIR(WHITE_BLUE_PAIR),
        // branch lightning, and impact flash using curses attron/mvaddch/refresh/napms
    }

    void animate_explosion(Vector2D center, int radius, GameContext& ctx)
    {
        // TODO: stub - explosion animation requires renderer replacement
        // Previously created a WINDOW* via newwin, drew fire characters with
        // expansion/peak/fade phases using wclear/wrefresh/napms/delwin
    }

    void animate_creature_hit(Vector2D position)
    {
        // TODO: stub - creature hit flash requires renderer replacement
        // Previously drew '~' at position with COLOR_PAIR(RED_YELLOW_PAIR) and napms(200)
    }
}
