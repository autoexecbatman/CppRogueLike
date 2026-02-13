// CharacterSheetUI.cpp - Handles character sheet display
#include <format>

#include "CharacterSheetUI.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Combat/WeaponDamageRegistry.h"

void CharacterSheetUI::display_character_sheet(const Player& player, GameContext& ctx)
{
    // TODO: render character sheet via new renderer
    void* character_sheet = nullptr;

    bool run = true;
    while (run)
    {
        // Display all sections (currently no-ops pending renderer)
        display_basic_info(character_sheet, player);
        display_experience_info(character_sheet, player, ctx);
        display_attributes(character_sheet, player, ctx);
        display_combat_stats(character_sheet, player, ctx);
        display_equipment_info(character_sheet, player, ctx);
        display_right_panel_info(character_sheet, player, ctx);
        display_constitution_effects(character_sheet, player, ctx);
        display_strength_effects(character_sheet, player, ctx);

        // TODO: wait for key input via new input system
        run = false;
    }

    cleanup_and_restore();
}

void CharacterSheetUI::display_basic_info(void* window, const Player& player)
{
    // TODO: render basic info via new renderer
    // Name, Class, Race, Level
    (void)window;
    (void)player;
}

void CharacterSheetUI::display_experience_info(void* window, const Player& player, GameContext& ctx)
{
    // Calculate XP needed for next level
    int currentXP = player.destructible->get_xp();
    int nextLevelXP = player.ai->get_next_level_xp(ctx, const_cast<Player&>(player));
    int xpNeeded = nextLevelXP - currentXP;
    float progressPercent = static_cast<float>(currentXP) / static_cast<float>(nextLevelXP) * 100.0f;

    // TODO: render experience info via new renderer
    (void)window;
    (void)xpNeeded;
    (void)progressPercent;
}

void CharacterSheetUI::display_attributes(void* window, const Player& player, GameContext& ctx)
{
    // Get strength modifiers from attributes table
    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);

    // Display Constitution with HP bonus effect
    int conBonus = get_constitution_bonus(player, ctx);

    // TODO: render attributes via new renderer
    (void)window;
    (void)strHitMod;
    (void)strDmgMod;
    (void)conBonus;
}

void CharacterSheetUI::display_combat_stats(void* window, const Player& player, GameContext& ctx)
{
    // Calculate base HP and Con bonus
    int baseHP = player.destructible->get_hp_base();
    int conBonusTotal = player.destructible->get_max_hp() - baseHP;

    // Display attack bonus from strength if applicable
    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);

    // Dexterity bonuses
    const auto& dexterityAttributes = ctx.data_manager->get_dexterity_attributes();
    int missileAdj = 0;
    int defensiveAdj = 0;
    if (player.get_dexterity() > 0 && player.get_dexterity() <= dexterityAttributes.size())
    {
        missileAdj = dexterityAttributes.at(player.get_dexterity() - 1).MissileAttackAdj;
        defensiveAdj = dexterityAttributes.at(player.get_dexterity() - 1).DefensiveAdj;
    }

    // TODO: render combat stats via new renderer
    (void)window;
    (void)baseHP;
    (void)conBonusTotal;
    (void)strHitMod;
    (void)strDmgMod;
    (void)missileAdj;
    (void)defensiveAdj;
}

void CharacterSheetUI::display_equipment_info(void* window, const Player& player, GameContext& ctx)
{
    // Get equipped weapon from proper slot-based equipment system
    auto* equippedWeapon = player.get_equipped_item(EquipmentSlot::RIGHT_HAND);

    // Get damage display string
    std::string damageDisplay;
    if (equippedWeapon && equippedWeapon->is_weapon())
    {
        const ItemEnhancement* enh = equippedWeapon->is_enhanced() ? &equippedWeapon->get_enhancement() : nullptr;
        damageDisplay = WeaponDamageRegistry::get_enhanced_damage_info(equippedWeapon->itemId, enh).displayRoll;
    }
    else
    {
        damageDisplay = WeaponDamageRegistry::get_unarmed_damage_info().displayRoll;
    }

    int strDmgMod = get_strength_damage_modifier(player, ctx);

    // TODO: render equipment info via new renderer
    (void)window;
    (void)equippedWeapon;
    (void)damageDisplay;
    (void)strDmgMod;
}

void CharacterSheetUI::display_right_panel_info(void* window, const Player& player, GameContext& ctx)
{
    // TODO: render right panel info via new renderer
    // Gender, Gold, Hunger display
    (void)window;
    (void)player;
    (void)ctx;
}

void CharacterSheetUI::display_constitution_effects(void* window, const Player& player, GameContext& ctx)
{
    // TODO: render constitution effects via new renderer
    (void)window;
    (void)player;
    (void)ctx;
}

void CharacterSheetUI::display_strength_effects(void* window, const Player& player, GameContext& ctx)
{
    // TODO: render strength effects via new renderer
    (void)window;
    (void)player;
    (void)ctx;
}

int CharacterSheetUI::get_strength_hit_modifier(const Player& player, GameContext& ctx)
{
    if (player.get_strength() > 0 && player.get_strength() <= ctx.data_manager->get_strength_attributes().size())
    {
        return ctx.data_manager->get_strength_attributes().at(player.get_strength() - 1).hitProb;
    }
    return 0;
}

int CharacterSheetUI::get_strength_damage_modifier(const Player& player, GameContext& ctx)
{
    if (player.get_strength() > 0 && player.get_strength() <= ctx.data_manager->get_strength_attributes().size())
    {
        return ctx.data_manager->get_strength_attributes().at(player.get_strength() - 1).dmgAdj;
    }
    return 0;
}

int CharacterSheetUI::get_constitution_bonus(const Player& player, GameContext& ctx)
{
    if (player.get_constitution() >= 1 && player.get_constitution() <= ctx.data_manager->get_constitution_attributes().size())
    {
        return ctx.data_manager->get_constitution_attributes().at(player.get_constitution() - 1).HPAdj;
    }
    return 0;
}

void CharacterSheetUI::cleanup_and_restore()
{
    // TODO: restore game display via new renderer
}
