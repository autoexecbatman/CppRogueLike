// file: MenuClass.cpp
#include <memory>
#include <vector>

#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "ListMenu.h"
#include "MenuClass.h"
#include "MenuName.h"

std::unique_ptr<BaseMenu> make_class_menu(GameContext& ctx)
{
    std::vector<MenuEntry> entries;

    auto fighterCommand = [](GameContext& ctx)
    {
        ctx.player->playerClass = "Fighter";
        ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
        ctx.player->set_creature_class(CreatureClass::FIGHTER);
        ctx.player->set_hit_die(10);
        // Starting gear is equipped in Game::update() STARTUP phase after init().
        ctx.menus->push_back(std::make_unique<MenuName>(ctx));
    };
    entries.push_back({ "Fighter", 'f', fighterCommand });

    auto rogueCommand = [](GameContext& ctx)
    {
        ctx.player->playerClass = "Rogue";
        ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
        ctx.player->set_creature_class(CreatureClass::ROGUE);
        ctx.player->set_hit_die(6);
        ctx.menus->push_back(std::make_unique<MenuName>(ctx));
    };
    entries.push_back({ "Rogue", 'r', rogueCommand });

    auto clericCommand = [](GameContext& ctx)
    {
        ctx.player->playerClass = "Cleric";
        ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
        ctx.player->set_creature_class(CreatureClass::CLERIC);
        ctx.player->set_hit_die(8);
        ctx.menus->push_back(std::make_unique<MenuName>(ctx));
    };
    entries.push_back({ "Cleric", 'c', clericCommand });

    auto wizardCommand = [](GameContext& ctx)
    {
        ctx.player->playerClass = "Wizard";
        ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
        ctx.player->set_creature_class(CreatureClass::WIZARD);
        ctx.player->set_hit_die(4);
        ctx.menus->push_back(std::make_unique<MenuName>(ctx));
    };
    entries.push_back({ "Wizard", 'z', wizardCommand });

    auto randomCommand = [](GameContext& ctx)
    {
        switch (ctx.dice->d4())
        {
        case 1:
            ctx.player->playerClass = "Fighter";
            ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
            ctx.player->set_creature_class(CreatureClass::FIGHTER);
            ctx.player->set_hit_die(10);
            break;
        case 2:
            ctx.player->playerClass = "Rogue";
            ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
            ctx.player->set_creature_class(CreatureClass::ROGUE);
            ctx.player->set_hit_die(6);
            break;
        case 3:
            ctx.player->playerClass = "Wizard";
            ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
            ctx.player->set_creature_class(CreatureClass::WIZARD);
            ctx.player->set_hit_die(4);
            break;
        case 4:
            ctx.player->playerClass = "Cleric";
            ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
            ctx.player->set_creature_class(CreatureClass::CLERIC);
            ctx.player->set_hit_die(8);
            break;
        default:
            break;
        }
        ctx.menus->push_back(std::make_unique<MenuName>(ctx));
    };
    entries.push_back({ "Random", 'x', randomCommand });

    auto backCommand = [](GameContext& ctx)
    {
        ctx.menus->back()->back = true;
    };
    entries.push_back({ "Back", 'b', backCommand });

    return std::make_unique<ListMenu>(
        "SELECT CLASS",
        std::move(entries),
        std::function<void(GameContext&)>{},
        std::function<void(GameContext&)>{},
        ctx);
}

// end of file: MenuClass.cpp
