// file: Menu.cpp
#include <memory>
#include <vector>

#include <raylib.h>

#include "../Core/GameContext.h"
#include "../Gui/Gui.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "ListMenu.h"
#include "Menu.h"
#include "MenuGender.h"

#ifndef EMSCRIPTEN
#include "../Tools/ItemEditor.h"
#include "../Tools/MonsterEditor.h"
#include "../Tools/RoomEditor.h"
#include "../Tools/SpellEditor.h"
#endif

std::unique_ptr<BaseMenu> make_main_menu(bool startup, GameContext& ctx)
{
    std::vector<MenuEntry> entries;

    auto newGameCommand = [](GameContext& ctx)
    {
        ctx.menuManager->set_game_initialized(false);
        ctx.menus->push_back(make_gender_menu(ctx));
    };
    entries.push_back({ "New Game", 'n', newGameCommand });

    if (GameStateManager::save_file_exists())
    {
        auto loadGameCommand = [](GameContext& ctx)
        {
            ctx.stateManager->load_all(ctx);
        };
        entries.push_back({ "Load Game", 'l', loadGameCommand });
    }

    auto optionsCommand = [startup](GameContext& ctx)
    {
        ctx.menus->push_back(make_main_menu(startup, ctx));
    };
    entries.push_back({ "Options", 'o', optionsCommand });

#ifndef EMSCRIPTEN
    auto roomEditorCommand = [](GameContext& ctx)
    {
        ctx.roomEditor->enter(*ctx.prefabLibrary);
    };
    entries.push_back({ "Room Editor", 'e', roomEditorCommand });

    auto monsterEditorCommand = [](GameContext& ctx)
    {
        ctx.monsterEditor->enter();
    };
    entries.push_back({ "Monster Editor", 'm', monsterEditorCommand });

    auto spellEditorCommand = [](GameContext& ctx)
    {
        ctx.spellEditor->enter();
    };
    entries.push_back({ "Spell Editor", 'p', spellEditorCommand });

    auto itemEditorCommand = [](GameContext& ctx)
    {
        ctx.itemEditor->enter(ctx);
    };
    entries.push_back({ "Item Editor", 'i', itemEditorCommand });
#endif

    auto quitCommand = [](GameContext& ctx)
    {
        ctx.gameState->set_run(false);
        ctx.gameState->set_should_save(false);
        ctx.messageSystem->log("You quit without saving!");
    };
    entries.push_back({ "Quit", 'q', quitCommand });

    // ESC on the startup menu quits the game; in-game it just closes.
    std::function<void(GameContext&)> onEscape{};
    if (startup)
    {
        onEscape = quitCommand;
    }

    // In-game menu renders the world behind it each frame.
    std::function<void(GameContext&)> onFrame{};
    if (!startup)
    {
        onFrame = [](GameContext& ctx)
        {
            if (ctx.menuManager->is_game_initialized())
            {
                ctx.renderingManager->render(ctx);
                ctx.gui->gui_render(ctx);
            }
        };
    }

    return std::make_unique<ListMenu>(
        "MAIN MENU",
        std::move(entries),
        std::move(onEscape),
        std::move(onFrame),
        ctx);
}

// end of file: Menu.cpp
