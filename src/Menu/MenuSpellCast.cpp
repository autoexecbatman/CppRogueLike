#include <algorithm>
#include <cassert>
#include <format>
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

    assert(ctx.renderer && "MenuSpellCast: renderer required before construction");
    const int width = 45;
    const int height = static_cast<int>(availableSpells.size()) + 4;
    const int vrows = ctx.renderer->get_viewport_rows();
    const int vcols = ctx.renderer->get_viewport_cols();
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
    menu_draw_box();
    menu_print(2, 1, "Cast Spell (ESC to cancel):");

    for (size_t i = 0; i < availableSpells.size(); ++i)
    {
        const auto& def = SpellSystem::get_by_key(availableSpells[i]);
        const char letter = static_cast<char>('a' + static_cast<int>(i));
        const int row = static_cast<int>(i) + 2;

        if (static_cast<int>(i) == selectedIndex)
        {
            menu_highlight_on();
        }

        if (!spellSources[i].empty())
        {
            menu_print(2, row, std::format("{}) {} [{}]", letter, def.name, spellSources[i]));
        }
        else
        {
            menu_print(2, row, std::format("{}) {} (L{})", letter, def.name, def.level));
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

    // onSuccess fires when the spell takes effect — immediately for instant spells,
    // deferred via TargetingMenu callback for targeted spells.
    // Init-capture &playerRef = player binds to the Player member (session lifetime),
    // not to a local alias that would dangle after handle_selection() returns.
    auto onSuccess = [key, isMemorized, &playerRef = player](GameContext& innerCtx)
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

void MenuSpellCast::on_key(GameKey key, int ch, GameContext& ctx)
{
    if (key == GameKey::ESCAPE)
    {
        menu_set_run_false();
    }
    else if (key == GameKey::UP)
    {
        if (selectedIndex > 0)
        {
            selectedIndex--;
        }
    }
    else if (key == GameKey::DOWN)
    {
        if (selectedIndex < static_cast<int>(availableSpells.size()) - 1)
        {
            selectedIndex++;
        }
    }
    else if (key == GameKey::ENTER || key == GameKey::SPACE)
    {
        handle_selection(ctx);
    }
    else if (ch >= 'a' && ch <= 'z')
    {
        int selection = ch - 'a';
        if (selection < static_cast<int>(availableSpells.size()))
        {
            selectedIndex = selection;
            handle_selection(ctx);
        }
    }
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
    on_key(lastKey, lastChar, ctx);
}
