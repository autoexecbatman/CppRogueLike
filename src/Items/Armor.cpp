#include "Armor.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

Armor::Armor(int acBonus) : armorClass{ acBonus }
{
}

bool Armor::use(Item& item, Creature& wearer, GameContext& ctx)
{
    std::string armorName = item.actorData.name;

    if (wearer.uniqueId == ctx.player->uniqueId)
    {
        Player* player = static_cast<Player*>(&wearer);

        bool wasEquipped = player->is_item_equipped(item.uniqueId);
        bool success = player->toggle_armor(item.uniqueId, ctx);

        if (success)
        {
            if (wasEquipped)
            {
                ctx.message_system->message(WHITE_BLACK_PAIR, "You remove the " + armorName + ".", true);
            }
            else
            {
                ctx.message_system->message(WHITE_BLACK_PAIR, "You put on the " + armorName + ".", true);
            }
        }

        return success;
    }

    apply_stat_effects(wearer, item, ctx);
    return true;
}

void Armor::apply_stat_effects(Creature& creature, Item& owner, GameContext& ctx)
{
    if (owner.has_state(ActorState::IS_EQUIPPED))
    {
        owner.remove_state(ActorState::IS_EQUIPPED);
        creature.destructible->update_armor_class(creature, ctx);
    }
    else
    {
        owner.add_state(ActorState::IS_EQUIPPED);
        creature.destructible->update_armor_class(creature, ctx);
    }
}

void Armor::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::ARMOR);
    j["armorClass"] = armorClass;
}

void Armor::load(const json& j)
{
    armorClass = j.at("armorClass").get<int>();
}
