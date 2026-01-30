#include "Jewelry.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

// ===== MAGICAL EQUIPMENT (Helms, Rings with special effects) =====

bool MagicalEquipment::use(Item& item, Creature& wearer, GameContext& ctx)
{
    const std::string& itemName = item.actorData.name;

    // For players, delegate to Equipment System
    if (wearer.uniqueId == ctx.player->uniqueId)
    {
        Player* player = static_cast<Player*>(&wearer);
        const bool wasEquipped = player->is_item_equipped(item.uniqueId);

        EquipmentSlot slot = get_equipment_slot();

        // For rings, intelligently choose available slot
        if (slot == EquipmentSlot::RIGHT_RING && !wasEquipped)
        {
            // If right ring is occupied, try left ring
            if (player->is_slot_occupied(EquipmentSlot::RIGHT_RING))
            {
                slot = EquipmentSlot::LEFT_RING;
            }
        }

        const bool success = player->toggle_equipment(item.uniqueId, slot, ctx);
        if (success)
        {
            if (wasEquipped)
            {
                // Unequipping - clear any active effects
                if (effect == MagicalEffect::INVISIBILITY && player->is_invisible())
                {
                    player->clear_invisible();
                    ctx.message_system->message(CYAN_BLACK_PAIR, "Your invisibility fades.", true);
                }
                ctx.message_system->message(WHITE_BLACK_PAIR, "You remove the " + itemName + ".", true);
            }
            else
            {
                // Equipping - special message for Ring of Invisibility
                if (effect == MagicalEffect::INVISIBILITY)
                {
                    ctx.message_system->message(CYAN_BLACK_PAIR, "The ring pulses with arcane power. Press Ctrl+C to cast.", true);
                }
                ctx.message_system->message(WHITE_BLACK_PAIR, "You put on the " + itemName + ".", true);
            }

            // Update AC if this is a protection ring or Helm of Brilliance
            if (MagicalEffectUtils::is_protection_effect(effect) || effect == MagicalEffect::BRILLIANCE)
            {
                player->destructible->update_armor_class(*player, ctx);
            }
        }
        return success;
    }

    // For NPCs, apply magical effect
    apply_effect(wearer, item, ctx);
    return true;
}

void MagicalEquipment::apply_effect(Creature& creature, Item& owner, GameContext& ctx)
{
    // Toggle equipped state
    // Actual effect behavior (telepathy, underwater action) is checked during gameplay
    // by querying equipped items rather than applying permanent stat changes
    if (!owner.has_state(ActorState::IS_EQUIPPED))
    {
        owner.add_state(ActorState::IS_EQUIPPED);
    }
    else
    {
        owner.remove_state(ActorState::IS_EQUIPPED);
    }
}

// ===== MAGICAL EQUIPMENT BASE (DRY) =====

void MagicalEquipment::save_magical_effect(json& j, PickableType type) const
{
    j["type"] = static_cast<int>(type);
    j["effect"] = static_cast<int>(effect);
}

void MagicalEquipment::load_magical_effect(const json& j)
{
    if (j.contains("effect"))
    {
        effect = static_cast<MagicalEffect>(j.at("effect").get<int>());
    }
}

// ===== MAGICAL HELMS =====

void MagicalHelm::save(json& j)
{
    save_magical_effect(j, PickableType::HELMET);
}

void MagicalHelm::load(const json& j)
{
    load_magical_effect(j);
}

EquipmentSlot MagicalHelm::get_equipment_slot() const
{
    return EquipmentSlot::HEAD;
}

// ===== MAGICAL RINGS =====

void MagicalRing::save(json& j)
{
    save_magical_effect(j, PickableType::RING);
}

void MagicalRing::load(const json& j)
{
    load_magical_effect(j);
}

EquipmentSlot MagicalRing::get_equipment_slot() const
{
    return EquipmentSlot::RIGHT_RING;
}

// ===== STAT BOOST EQUIPMENT (Gauntlets, Girdles) =====

bool StatBoostEquipment::use(Item& item, Creature& wearer, GameContext& ctx)
{
    const std::string& itemName = item.actorData.name;

    if (wearer.uniqueId == ctx.player->uniqueId)
    {
        Player* player = static_cast<Player*>(&wearer);
        const EquipmentSlot slot = get_equipment_slot();
        const bool wasEquipped = player->is_item_equipped(item.uniqueId);

        const bool success = player->toggle_equipment(item.uniqueId, slot, ctx);
        if (success)
        {
            if (wasEquipped)
            {
                // Restore/remove stats
                if (is_set_mode)
                {
                    if (str_bonus != 0) player->set_strength(original_stats.str);
                    if (dex_bonus != 0) player->set_dexterity(original_stats.dex);
                    if (con_bonus != 0) player->set_constitution(original_stats.con);
                    if (int_bonus != 0) player->set_intelligence(original_stats.intel);
                    if (wis_bonus != 0) player->set_wisdom(original_stats.wis);
                    if (cha_bonus != 0) player->set_charisma(original_stats.cha);
                }
                else
                {
                    player->set_strength(player->get_strength() - str_bonus);
                    player->set_dexterity(player->get_dexterity() - dex_bonus);
                    player->set_constitution(player->get_constitution() - con_bonus);
                    player->set_intelligence(player->get_intelligence() - int_bonus);
                    player->set_wisdom(player->get_wisdom() - wis_bonus);
                    player->set_charisma(player->get_charisma() - cha_bonus);
                }

                player->destructible->update_armor_class(*player, ctx);
                ctx.message_system->message(WHITE_BLACK_PAIR, "You remove the " + itemName + ".", true);
            }
            else
            {
                // Apply/set stats
                if (is_set_mode)
                {
                    if (str_bonus != 0) { original_stats.str = player->get_strength(); player->set_strength(str_bonus); }
                    if (dex_bonus != 0) { original_stats.dex = player->get_dexterity(); player->set_dexterity(dex_bonus); }
                    if (con_bonus != 0) { original_stats.con = player->get_constitution(); player->set_constitution(con_bonus); }
                    if (int_bonus != 0) { original_stats.intel = player->get_intelligence(); player->set_intelligence(int_bonus); }
                    if (wis_bonus != 0) { original_stats.wis = player->get_wisdom(); player->set_wisdom(wis_bonus); }
                    if (cha_bonus != 0) { original_stats.cha = player->get_charisma(); player->set_charisma(cha_bonus); }
                }
                else
                {
                    player->set_strength(player->get_strength() + str_bonus);
                    player->set_dexterity(player->get_dexterity() + dex_bonus);
                    player->set_constitution(player->get_constitution() + con_bonus);
                    player->set_intelligence(player->get_intelligence() + int_bonus);
                    player->set_wisdom(player->get_wisdom() + wis_bonus);
                    player->set_charisma(player->get_charisma() + cha_bonus);
                }

                player->destructible->update_armor_class(*player, ctx);
                ctx.message_system->message(WHITE_BLACK_PAIR, "You put on the " + itemName + ".", true);
            }
        }
        return success;
    }

    apply_stat_bonuses(wearer, item, ctx);
    return true;
}

void StatBoostEquipment::apply_stat_bonuses(Creature& creature, Item& owner, GameContext& ctx)
{
    if (owner.has_state(ActorState::IS_EQUIPPED))
    {
        owner.remove_state(ActorState::IS_EQUIPPED);

        // Restore/remove stats
        if (is_set_mode)
        {
            if (str_bonus != 0) creature.set_strength(original_stats.str);
            if (dex_bonus != 0) creature.set_dexterity(original_stats.dex);
            if (con_bonus != 0) creature.set_constitution(original_stats.con);
            if (int_bonus != 0) creature.set_intelligence(original_stats.intel);
            if (wis_bonus != 0) creature.set_wisdom(original_stats.wis);
            if (cha_bonus != 0) creature.set_charisma(original_stats.cha);
        }
        else
        {
            creature.set_strength(creature.get_strength() - str_bonus);
            creature.set_dexterity(creature.get_dexterity() - dex_bonus);
            creature.set_constitution(creature.get_constitution() - con_bonus);
            creature.set_intelligence(creature.get_intelligence() - int_bonus);
            creature.set_wisdom(creature.get_wisdom() - wis_bonus);
            creature.set_charisma(creature.get_charisma() - cha_bonus);
        }

        creature.destructible->update_armor_class(creature, ctx);
    }
    else
    {
        owner.add_state(ActorState::IS_EQUIPPED);

        // Apply/set stats
        if (is_set_mode)
        {
            if (str_bonus != 0) { original_stats.str = creature.get_strength(); creature.set_strength(str_bonus); }
            if (dex_bonus != 0) { original_stats.dex = creature.get_dexterity(); creature.set_dexterity(dex_bonus); }
            if (con_bonus != 0) { original_stats.con = creature.get_constitution(); creature.set_constitution(con_bonus); }
            if (int_bonus != 0) { original_stats.intel = creature.get_intelligence(); creature.set_intelligence(int_bonus); }
            if (wis_bonus != 0) { original_stats.wis = creature.get_wisdom(); creature.set_wisdom(wis_bonus); }
            if (cha_bonus != 0) { original_stats.cha = creature.get_charisma(); creature.set_charisma(cha_bonus); }
        }
        else
        {
            creature.set_strength(creature.get_strength() + str_bonus);
            creature.set_dexterity(creature.get_dexterity() + dex_bonus);
            creature.set_constitution(creature.get_constitution() + con_bonus);
            creature.set_intelligence(creature.get_intelligence() + int_bonus);
            creature.set_wisdom(creature.get_wisdom() + wis_bonus);
            creature.set_charisma(creature.get_charisma() + cha_bonus);
        }

        creature.destructible->update_armor_class(creature, ctx);
    }
}

// ===== STAT BOOST EQUIPMENT BASE (DRY) =====

void StatBoostEquipment::save_stat_bonuses(json& j, PickableType type) const
{
    j["type"] = static_cast<int>(type);
    j["str_bonus"] = str_bonus;
    j["dex_bonus"] = dex_bonus;
    j["con_bonus"] = con_bonus;
    j["int_bonus"] = int_bonus;
    j["wis_bonus"] = wis_bonus;
    j["cha_bonus"] = cha_bonus;
    j["is_set_mode"] = is_set_mode;

    j["original_stats"] = {
        {"str", original_stats.str},
        {"dex", original_stats.dex},
        {"con", original_stats.con},
        {"intel", original_stats.intel},
        {"wis", original_stats.wis},
        {"cha", original_stats.cha}
    };
}

void StatBoostEquipment::load_stat_bonuses(const json& j)
{
    str_bonus = j.contains("str_bonus") ? j.at("str_bonus").get<int>() : 0;
    dex_bonus = j.contains("dex_bonus") ? j.at("dex_bonus").get<int>() : 0;
    con_bonus = j.contains("con_bonus") ? j.at("con_bonus").get<int>() : 0;
    int_bonus = j.contains("int_bonus") ? j.at("int_bonus").get<int>() : 0;
    wis_bonus = j.contains("wis_bonus") ? j.at("wis_bonus").get<int>() : 0;
    cha_bonus = j.contains("cha_bonus") ? j.at("cha_bonus").get<int>() : 0;
    is_set_mode = j.contains("is_set_mode") ? j.at("is_set_mode").get<bool>() : false;

    if (j.contains("original_stats"))
    {
        const auto& orig = j.at("original_stats");
        original_stats.str = orig.contains("str") ? orig.at("str").get<int>() : 0;
        original_stats.dex = orig.contains("dex") ? orig.at("dex").get<int>() : 0;
        original_stats.con = orig.contains("con") ? orig.at("con").get<int>() : 0;
        original_stats.intel = orig.contains("intel") ? orig.at("intel").get<int>() : 0;
        original_stats.wis = orig.contains("wis") ? orig.at("wis").get<int>() : 0;
        original_stats.cha = orig.contains("cha") ? orig.at("cha").get<int>() : 0;
    }
}

// ===== AMULET =====

void JewelryAmulet::save(json& j)
{
    save_stat_bonuses(j, PickableType::AMULET);
}

void JewelryAmulet::load(const json& j)
{
    load_stat_bonuses(j);
}

EquipmentSlot JewelryAmulet::get_equipment_slot() const
{
    return EquipmentSlot::NECK;
}

// ===== GAUNTLETS =====

void Gauntlets::save(json& j)
{
    save_stat_bonuses(j, PickableType::GAUNTLETS);
}

void Gauntlets::load(const json& j)
{
    load_stat_bonuses(j);
}

EquipmentSlot Gauntlets::get_equipment_slot() const
{
    return EquipmentSlot::GAUNTLETS;
}

// ===== GIRDLE =====

void Girdle::save(json& j)
{
    save_stat_bonuses(j, PickableType::GIRDLE);
}

void Girdle::load(const json& j)
{
    load_stat_bonuses(j);
}

EquipmentSlot Girdle::get_equipment_slot() const
{
    return EquipmentSlot::GIRDLE;
}
