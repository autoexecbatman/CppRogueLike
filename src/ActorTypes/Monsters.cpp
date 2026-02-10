// file: Monsters.cpp
#include <memory>

#include "Monsters.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMimic.h"
#include "../Actor/Destructible.h"
#include "../Actor/Attacker.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Combat/DamageInfo.h"
#include "../Factories/ItemCreator.h"
#include "../Items/ItemClassification.h"

Mimic::Mimic(Vector2D position, GameContext& ctx) : Creature(position, ActorData{ 'M', "mimic", RED_YELLOW_PAIR })
{
    const int hp = ctx.dice->d6() + ctx.dice->d4();
    const int thaco = 17;
    const int ac = 7;

    // Mimic: AD&D 2e — strong pseudopod, average dex, tough, low animal INT,
    // decent predator WIS, very low CHA (horrifying when revealed).
    set_strength(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6() + 2); // 3d6+2 avg 12 — strong grip
    set_dexterity(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6());    // 3d6    avg 10 — average
    set_constitution(ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()); // 3d6    avg 10 — average
    set_intelligence(ctx.dice->d4() + 2);                               // 1d4+2  avg  4 — low cunning
    set_wisdom(ctx.dice->d6() + ctx.dice->d6() + 1);                    // 2d6+1  avg  8 — ambush instinct
    set_charisma(ctx.dice->d4());                                        // 1d4    avg  2 — terrifying

    set_weapon_equipped("Pseudopod");

    attacker = std::make_unique<Attacker>(DamageValues::Dagger());
    destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead mimic", 150, thaco, ac);

    ai = std::make_unique<AiMimic>();

    initDisguises();

    if (!possibleDisguises.empty())
    {
        const size_t index = ctx.dice->roll(0, static_cast<int>(possibleDisguises.size()) - 1);
        const auto& chosen = possibleDisguises.at(index);
        actorData.ch = chosen.ch;
        actorData.name = chosen.name;
        actorData.color = chosen.color;
    }
    else
    {
        throw ("possibleDisguises is empty from initDisguises()!");
    }

    remove_state(ActorState::BLOCKS);
}

void Mimic::initDisguises()
{
    // Pull symbol/name/color directly from the item registry — single source of truth.
    auto from_item = [](ItemId id) -> Disguise
    {
        const auto& p = ItemCreator::get_params(id);
        return { p.symbol, std::string(p.name), p.color };
    };

    possibleDisguises =
    {
        from_item(ItemId::GOLD),
        from_item(ItemId::HEALTH_POTION),
        from_item(ItemId::SCROLL_LIGHTNING),
        from_item(ItemId::SHORT_SWORD),
        from_item(ItemId::FOOD_RATION),
    };
}

std::vector<Disguise> Mimic::get_possible_disguises() const
{
    return possibleDisguises;
}
