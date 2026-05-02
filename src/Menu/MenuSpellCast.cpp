#include <algorithm>
#include <string>

#include <raylib.h>

#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/SpellSystem.h"
#include "MenuSpellCast.h"

MenuSpellCast::MenuSpellCast(Player& player, GameContext& ctx)
    : player(player)
{
    populate_spells();

    if (availableSpells.empty())
    {
        // No spells available — menu() will close immediately on first frame.
        return;
    }

    const int width = 45;
    const int height = static_cast<int>(availableSpells.size()) + 4;
    const int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 30;
    const int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 119;
    const size_t startX = static_cast<size_t>((vcols - width) / 2);
    const size_t startY = static_cast<size_t>((vrows - height) / 2);
    menu_new(static_cast<size_t>(width), static_cast<size_t>(height), startX, startY, ctx);
}

void MenuSpellCast::populate_spells()
{
    availableSpells.clear();
    spellSources.clear();

    // Add memorized spells (string keys)
    for (const auto& key : player.memorizedSpells)
    {
        availableSpells.push_back(key);
        spellSources.push_back("");
    }

    // Add item-granted spells
    const auto itemSpells = SpellSystem::get_item_granted_spells(player);
    for (const auto& itemSpell : itemSpells)
    {
        availableSpells.push_back(itemSpell.key);
        spellSources.push_back(itemSpell.source);
    }
}

void MenuSpellCast::draw()
{
    menu_clear();
    menu_print(2, 1, "Cast Spell (ESC to cancel):");

    for (size_t i = 0; i < availableSpells.size(); ++i)
    {
        const auto& def = SpellSystem::get_by_key(availableSpells[i]);

        if (static_cast<int>(i) == selectedIndex)
        {
            menu_highlight_on();
        }

        if (!spellSources[i].empty())
        {
            menu_print(2, static_cast<int>(i) + 2, std::string(1, 'a' + static_cast<char>(i)) + ") " + def.name + " [" + spellSources[i] + "]");
        }
        else
        {
            menu_print(2, static_cast<int>(i) + 2, std::string(1, 'a' + static_cast<char>(i)) + ") " + def.name + " (L" + std::to_string(def.level) + ")");
        }

        if (static_cast<int>(i) == selectedIndex)
        {
            menu_highlight_off();
        }
    }

    menu_refresh();
}

void MenuSpellCast::handle_selection(GameContext& ctx)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(availableSpells.size()))
    {
        return;
    }

    const std::string key = availableSpells[selectedIndex];
    const bool isMemorized = spellSources[selectedIndex].empty();
    Player& playerRef = player;

    // onSuccess fires when the spell takes effect — immediately for instant spells,
    // deferred via TargetingMenu callback for targeted spells.
    auto onSuccess = [key, isMemorized, &playerRef](GameContext& innerCtx)
    {
        if (isMemorized)
        {
            std::erase(playerRef.memorizedSpells, key);
        }
        innerCtx.gameState->set_game_status(GameStatus::NEW_TURN);
    };

    SpellSystem::cast_spell_by_key(key, player, std::move(onSuccess), ctx);
    menu_set_run_false();
}

void MenuSpellCast::menu(GameContext& ctx)
{
    if (availableSpells.empty())
    {
        ctx.messageSystem->message(WHITE_BLACK_PAIR, "No spells available.", true);
        menu_set_run_false();
        return;
    }

    menu_key_listen();
    draw();

    switch (keyPress)
    {
    case 27: // ESC
    {
        menu_set_run_false();
        break;
    }

    case 0x103: // UP
    {
        if (selectedIndex > 0)
        {
            selectedIndex--;
        }
        break;
    }

    case 0x102: // DOWN
    {
        if (selectedIndex < static_cast<int>(availableSpells.size()) - 1)
        {
            selectedIndex++;
        }
        break;
    }

    case '\n': // Enter
    case ' ':  // Space
    {
        handle_selection(ctx);
        break;
    }

    default:
    {
        // Letter selection (a-z)
        if (keyPress >= 'a' && keyPress <= 'z')
        {
            int selection = keyPress - 'a';
            if (selection >= 0 && selection < static_cast<int>(availableSpells.size()))
            {
                selectedIndex = selection;
                handle_selection(ctx);
            }
        }
        break;
    }
    }
}
