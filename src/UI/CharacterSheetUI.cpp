// CharacterSheetUI.cpp - Handles character sheet display
#include <format>

#include "CharacterSheetUI.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"

void CharacterSheetUI::display_character_sheet(const Player& player, GameContext& ctx)
{
    bool run = true;
    while (run && !WindowShouldClose())
    {
        ctx.renderer->begin_frame();

        int ts       = ctx.renderer->get_tile_size();
        int font_off = (ts - ctx.renderer->get_font_size()) / 2;
        int vcols    = ctx.renderer->get_viewport_cols();
        int vrows    = ctx.renderer->get_viewport_rows();

        ctx.renderer->draw_frame(0, 0, vcols, vrows);

        std::string_view title  = "CHARACTER SHEET";
        int title_w = ctx.renderer->measure_text(title);
        int title_x = (vcols * ts - title_w) / 2;
        ctx.renderer->draw_text(title_x, font_off, title, YELLOW_BLACK_PAIR);

        // Hint in bottom border row
        std::string_view hint = "[ESC] or [SPACE] to close";
        int hint_w = ctx.renderer->measure_text(hint);
        int hint_x = (vcols * ts - hint_w) / 2;
        ctx.renderer->draw_text(hint_x, (vrows - 1) * ts + font_off, hint, CYAN_BLACK_PAIR);

        int row = 1;
        display_basic_info(player, ctx, row);
        display_experience_info(player, ctx, row);
        display_attributes(player, ctx, row);
        display_combat_stats(player, ctx, row);
        display_equipment_info(player, ctx, row);
        display_right_panel_info(player, ctx, row);

        ctx.renderer->end_frame();

        ctx.input_system->poll();
        GameKey key = ctx.input_system->get_key();
        if (key == GameKey::ESCAPE || key == GameKey::SPACE)
            run = false;
    }
}

void CharacterSheetUI::display_basic_info(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    std::string line = std::format(
        "Name: {}   Class: {}   Race: {}   Level: {}",
        player.get_name(),
        player.playerClass,
        player.playerRace,
        player.get_level()
    );
    ctx.renderer->draw_text(x, row * ts + font_off, line, WHITE_BLACK_PAIR);
    row += 2;
}

void CharacterSheetUI::display_experience_info(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    int currentXP   = player.destructible->get_xp();
    int nextLevelXP = player.ai->get_next_level_xp(ctx, const_cast<Player&>(player));
    int xpNeeded    = nextLevelXP - currentXP;

    ctx.renderer->draw_text(
        x,
        row * ts + font_off,
        std::format("XP: {} / {}   (Need: {})", currentXP, nextLevelXP, xpNeeded),
        CYAN_BLACK_PAIR
    );
    row += 2;
}

void CharacterSheetUI::display_attributes(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);
    int conBonus  = get_constitution_bonus(player, ctx);

    int missileAdj   = 0;
    int defensiveAdj = 0;
    const auto& dexAttr = ctx.data_manager->get_dexterity_attributes();
    if (player.get_dexterity() > 0 &&
        player.get_dexterity() <= static_cast<int>(dexAttr.size()))
    {
        missileAdj   = dexAttr.at(player.get_dexterity() - 1).MissileAttackAdj;
        defensiveAdj = dexAttr.at(player.get_dexterity() - 1).DefensiveAdj;
    }

    ctx.renderer->draw_text(x, row * ts + font_off, "--- ATTRIBUTES ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("STR: {:2d}  ({:+d} hit, {:+d} dmg)", player.get_strength(), strHitMod, strDmgMod),
        WHITE_BLACK_PAIR
    );
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("DEX: {:2d}  ({:+d} missile, {:+d} defensive)", player.get_dexterity(), missileAdj, defensiveAdj),
        WHITE_BLACK_PAIR
    );
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("CON: {:2d}  ({:+d} HP/level)", player.get_constitution(), conBonus),
        WHITE_BLACK_PAIR
    );
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("INT: {:2d}   WIS: {:2d}   CHA: {:2d}",
            player.get_intelligence(), player.get_wisdom(), player.get_charisma()),
        WHITE_BLACK_PAIR
    );
    row += 2;
}

void CharacterSheetUI::display_combat_stats(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    int hp            = player.destructible->get_hp();
    int maxHp         = player.destructible->get_max_hp();
    int baseHP        = player.destructible->get_hp_base();
    int conBonusTotal = maxHp - baseHP;

    ctx.renderer->draw_text(x, row * ts + font_off, "--- COMBAT ---", YELLOW_BLACK_PAIR);
    row++;

    int hpColor = (hp > maxHp / 2)
        ? GREEN_BLACK_PAIR
        : (hp > maxHp / 4 ? YELLOW_BLACK_PAIR : RED_BLACK_PAIR);

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("HP: {} / {}  (Base: {}, Con Bonus: {:+d})", hp, maxHp, baseHP, conBonusTotal),
        hpColor
    );
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("THAC0: {}   AC: {}   DR: {}",
            player.destructible->get_thaco(),
            player.destructible->get_armor_class(),
            player.destructible->get_dr()),
        WHITE_BLACK_PAIR
    );
    row += 2;
}

void CharacterSheetUI::display_equipment_info(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    auto* equippedWeapon = player.get_equipped_item(EquipmentSlot::RIGHT_HAND);

    std::string damageDisplay;
    if (equippedWeapon && equippedWeapon->is_weapon())
    {
        const ItemEnhancement* enh = equippedWeapon->is_enhanced()
            ? &equippedWeapon->get_enhancement()
            : nullptr;
        damageDisplay = WeaponDamageRegistry::get_enhanced_damage_info(
            equippedWeapon->itemId, enh).displayRoll;
    }
    else
    {
        damageDisplay = WeaponDamageRegistry::get_unarmed_damage_info().displayRoll;
    }

    int strDmgMod = get_strength_damage_modifier(player, ctx);

    ctx.renderer->draw_text(x, row * ts + font_off, "--- EQUIPMENT ---", YELLOW_BLACK_PAIR);
    row++;

    std::string weaponName = equippedWeapon
        ? std::string(equippedWeapon->get_name())
        : "(unarmed)";

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("Weapon: {}   Damage: {}  (STR bonus: {:+d})", weaponName, damageDisplay, strDmgMod),
        WHITE_BLACK_PAIR
    );
    row += 2;
}

void CharacterSheetUI::display_right_panel_info(const Player& player, GameContext& ctx, int& row)
{
    int ts       = ctx.renderer->get_tile_size();
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int x        = ts;

    std::string hungerStr = ctx.hunger_system
        ? ctx.hunger_system->get_hunger_state_string()
        : "Unknown";

    ctx.renderer->draw_text(x, row * ts + font_off, "--- OTHER ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        x, row * ts + font_off,
        std::format("Gender: {}   Gold: {} gp   Hunger: {}",
            player.get_gender(), player.get_gold(), hungerStr),
        WHITE_BLACK_PAIR
    );
    row += 2;
}

int CharacterSheetUI::get_strength_hit_modifier(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.data_manager->get_strength_attributes();
    if (player.get_strength() > 0 &&
        player.get_strength() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_strength() - 1).hitProb;
    }
    return 0;
}

int CharacterSheetUI::get_strength_damage_modifier(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.data_manager->get_strength_attributes();
    if (player.get_strength() > 0 &&
        player.get_strength() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_strength() - 1).dmgAdj;
    }
    return 0;
}

int CharacterSheetUI::get_constitution_bonus(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.data_manager->get_constitution_attributes();
    if (player.get_constitution() >= 1 &&
        player.get_constitution() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_constitution() - 1).HPAdj;
    }
    return 0;
}
