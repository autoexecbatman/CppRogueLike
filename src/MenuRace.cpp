// file: MenuRace.cpp
#include <memory>
#include <vector>

#include "GameContext.h"
#include "RandomDice.h"
#include "ListMenu.h"
#include "MenuClass.h"
#include "Player.h"
#include "MenuRace.h"

std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx)
{
    std::vector<MenuEntry> entries;

    auto humanCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Human";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HUMAN);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Human", 'h', humanCommand });

    auto dwarfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Dwarf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::DWARF);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Dwarf", 'd', dwarfCommand });

    auto elfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Elf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::ELF);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Elf", 'e', elfCommand });

    auto gnomeCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Gnome";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::GNOME);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Gnome", 'g', gnomeCommand });

    auto halfElfCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Half-Elf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HALFELF);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Half-Elf", 0, halfElfCommand });

    auto halflingCommand = [](GameContext& ctx)
    {
        ctx.playerBlueprint->playerRace = "Halfling";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HALFLING);
        ctx.menus->push_back(make_class_menu(ctx));
    };
    entries.push_back({ "Halfling", 'l', halflingCommand });

    auto randomCommand = [](GameContext& ctx)
    {
        switch (ctx.dice->d6())
        {
        case 1:
            ctx.playerBlueprint->playerRace = "Human";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HUMAN);
            break;
        case 2:
            ctx.playerBlueprint->playerRace = "Dwarf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::DWARF);
            break;
        case 3:
            ctx.playerBlueprint->playerRace = "Elf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::ELF);
            break;
        case 4:
            ctx.playerBlueprint->playerRace = "Gnome";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::GNOME);
            break;
        case 5:
            ctx.playerBlueprint->playerRace = "Half-Elf";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HALFELF);
            break;
        case 6:
            ctx.playerBlueprint->playerRace = "Halfling";
        ctx.playerBlueprint->racialModifier = racial_ability_modifiers(Player::PlayerRaceState::HALFLING);
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
