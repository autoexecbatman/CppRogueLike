// file: MenuRace.cpp
#include <memory>
#include <vector>

#include "../ActorTypes/Player.h"
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
        ctx.player->playerRace = "Human";
        ctx.player->playerRaceState = Player::PlayerRaceState::HUMAN;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Human", 'h', humanCommand });

    auto dwarfCommand = [](GameContext& ctx)
    {
        ctx.player->playerRace = "Dwarf";
        ctx.player->playerRaceState = Player::PlayerRaceState::DWARF;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Dwarf", 'd', dwarfCommand });

    auto elfCommand = [](GameContext& ctx)
    {
        ctx.player->playerRace = "Elf";
        ctx.player->playerRaceState = Player::PlayerRaceState::ELF;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Elf", 'e', elfCommand });

    auto gnomeCommand = [](GameContext& ctx)
    {
        ctx.player->playerRace = "Gnome";
        ctx.player->playerRaceState = Player::PlayerRaceState::GNOME;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Gnome", 'g', gnomeCommand });

    auto halfElfCommand = [](GameContext& ctx)
    {
        ctx.player->playerRace = "Half-Elf";
        ctx.player->playerRaceState = Player::PlayerRaceState::HALFELF;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Half-Elf", 0, halfElfCommand });  // no unambiguous hotkey

    auto halflingCommand = [](GameContext& ctx)
    {
        ctx.player->playerRace = "Halfling";
        ctx.player->playerRaceState = Player::PlayerRaceState::HALFLING;
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Halfling", 'l', halflingCommand });

    auto randomCommand = [](GameContext& ctx)
    {
        switch (ctx.dice->d6())
        {
        case 1:
            ctx.player->playerRace = "Human";
            ctx.player->playerRaceState = Player::PlayerRaceState::HUMAN;
            break;
        case 2:
            ctx.player->playerRace = "Dwarf";
            ctx.player->playerRaceState = Player::PlayerRaceState::DWARF;
            break;
        case 3:
            ctx.player->playerRace = "Elf";
            ctx.player->playerRaceState = Player::PlayerRaceState::ELF;
            break;
        case 4:
            ctx.player->playerRace = "Gnome";
            ctx.player->playerRaceState = Player::PlayerRaceState::GNOME;
            break;
        case 5:
            ctx.player->playerRace = "Half-Elf";
            ctx.player->playerRaceState = Player::PlayerRaceState::HALFELF;
            break;
        case 6:
            ctx.player->playerRace = "Halfling";
            ctx.player->playerRaceState = Player::PlayerRaceState::HALFLING;
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
