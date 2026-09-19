#include <algorithm>
#include <cassert>
#include <format>
#include <string>

#include <raylib.h>

#include "Player.h"
#include "Colors.h"
#include "GameContext.h"
#include "MessageSystem.h"
#include "SpellRegistry.h"
#include "SpellSystem.h"
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
    const int tileSize = ctx.renderer->get_tile_size();

    // 45 was a character count handed to menu_new, which reads tiles: 2880 pixels
    // on a 1280-pixel screen. Measured off the lines this menu actually draws.
    int widestText = ctx.renderer->measure_text("Cast Spell (ESC to cancel):");
    for (size_t i = 0; i < availableSpells.size(); ++i)
    {
        widestText = std::max(widestText, ctx.renderer->measure_text(spell_line(i, *ctx.spellRegistry)));
    }

    // One header row, then a row per spell.
    const int width = panel_tiles_for_text_width(widestText, tileSize);
    const int height = panel_tiles_for_text_rows(
        static_cast<int>(availableSpells.size()) + 1, tileSize, ctx.renderer->get_font_size());
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

std::string MenuSpellCast::spell_line(size_t index, const SpellRegistry& spells) const
{
    const SpellDefinition& definition = spells.get_by_key(availableSpells[index]);
    const char letter = static_cast<char>('a' + static_cast<int>(index));

    // An item-granted spell names what granted it; a memorised one names its level.
    if (!spellSources[index].empty())
    {
        return std::format("{}) {} [{}]", letter, definition.name, spellSources[index]);
    }
    return std::format("{}) {} (L{})", letter, definition.name, definition.level);
}

void MenuSpellCast::draw(const SpellRegistry& spells)
{
    menu_clear();
    menu_draw_box();
    menu_print(1, 0, "Cast Spell (ESC to cancel):");

    for (size_t i = 0; i < availableSpells.size(); ++i)
    {
        const int row = static_cast<int>(i) + 1;

        if (static_cast<int>(i) == selectedIndex)
        {
            menu_highlight_on();
        }

        menu_print(1, row, spell_line(i, spells));

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

void MenuSpellCast::on_key(GameContext& ctx)
{
    if (lastKey == GameKey::ESCAPE)
    {
        menu_set_run_false();
    }
    else if (lastKey == GameKey::UP)
    {
        if (selectedIndex > 0)
        {
            selectedIndex--;
        }
    }
    else if (lastKey == GameKey::DOWN)
    {
        if (selectedIndex < static_cast<int>(availableSpells.size()) - 1)
        {
            selectedIndex++;
        }
    }
    else if (lastKey == GameKey::ENTER || lastKey == GameKey::SPACE)
    {
        handle_selection(ctx);
    }
    else if (lastChar >= 'a' && lastChar <= 'z')
    {
        int selection = lastChar - 'a';
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
    draw(*ctx.spellRegistry);
    on_key(ctx);
}
