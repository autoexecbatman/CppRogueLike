// file: MenuRace.cpp
#include <memory>
#include <vector>

#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "ListMenu.h"
#include "MenuClass.h"
#include "MenuRace.h"

std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx)
{
    std::vector<MenuEntry> entries;

    auto humanCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Human";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Human", 'h', humanCommand });

    auto dwarfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Dwarf";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Dwarf", 'd', dwarfCommand });

    auto elfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Elf";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Elf", 'e', elfCommand });

    auto gnomeCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Gnome";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Gnome", 'g', gnomeCommand });

    auto halfElfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Half-Elf";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Half-Elf", 0, halfElfCommand });

    auto halflingCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint.playerRace = "Halfling";
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Halfling", 'l', halflingCommand });

    auto randomCommand = [](GameContext& ctx)
    {
        switch (ctx.dice->d6())
        {
        case 1:
            ctx.playerBlueprint.playerRace = "Human";
            break;
        case 2:
            ctx.playerBlueprint.playerRace = "Dwarf";
            break;
        case 3:
            ctx.playerBlueprint.playerRace = "Elf";
            break;
        case 4:
            ctx.playerBlueprint.playerRace = "Gnome";
            break;
        case 5:
            ctx.playerBlueprint.playerRace = "Half-Elf";
            break;
        case 6:
            ctx.playerBlueprint.playerRace = "Halfling";
            break;
        default:
            break;
        }
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Random", 'r', randomCommand });

    auto backCommand = [](GameContext& ctx)
    {
        ctx.menus->back()->back = true;
    };
    entries.push_back({ "Back", 'b', backCommand });

    return std::make_unique<ListMenu>(
        "SELECT RACE",
        std::move(entries),
        std::function<void(GameContext&)>{},
        std::function<void(GameContext&)>{},
        ctx);
}

// end of file: MenuRace.cpp
