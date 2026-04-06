// file: MenuGender.cpp
#include <memory>
#include <vector>

#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "ListMenu.h"
#include "MenuGender.h"
#include "MenuRace.h"

std::unique_ptr<BaseMenu> make_gender_menu(GameContext& ctx)
{
    std::vector<MenuEntry> entries;

    auto maleCommand = [](GameContext& ctx)
    {
        ctx.player->set_gender("Male");
        ctx.menus->push_back(make_race_menu(ctx));
    };
    entries.push_back({ "Male", 'm', maleCommand });

    auto femaleCommand = [](GameContext& ctx)
    {
        ctx.player->set_gender("Female");
        ctx.menus->push_back(make_race_menu(ctx));
    };
    entries.push_back({ "Female", 'f', femaleCommand });

    auto randomCommand = [](GameContext& ctx)
    {
        ctx.player->set_gender(ctx.dice->d2() == 1 ? "Male" : "Female");
        ctx.menus->push_back(make_race_menu(ctx));
    };
    entries.push_back({ "Random", 'r', randomCommand });

    auto backCommand = [](GameContext& ctx)
    {
        ctx.menus->back()->back = true;
    };
    entries.push_back({ "Back", 'b', backCommand });

    return std::make_unique<ListMenu>(
        "SELECT GENDER",
        std::move(entries),
        std::function<void(GameContext&)>{},
        std::function<void(GameContext&)>{},
        ctx);
}

// end of file: MenuGender.cpp
