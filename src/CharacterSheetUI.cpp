// CharacterSheetUI.cpp - Handles character sheet display
#include <format>
#include <string>

#include <raylib.h>

#include "EquipmentSlot.h"
#include "Player.h"
#include "Colors.h"
#include "WeaponDamageRegistry.h"
#include "GameContext.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "DataManager.h"
#include "DexterityAttributes.h"
#include "HungerSystem.h"
#include "StrengthAttributes.h"
#include "ItemEnhancements.h"
#include "CharacterSheetUI.h"

// ============================================================================
// Private helpers — not visible outside this translation unit.
// ============================================================================

namespace
{

// Strength as the book writes it: 18/76, 18/00 for a percentile of 100, a plain score otherwise.
//
// Example:
//   strength_text(player);   // -> "18/76", "18/00" or "17"
std::string strength_text(const Player& player)
{
    const int exceptional = player.get_exceptional_strength();
    if (player.get_strength() != 18 || exceptional < 1)
    {
        return std::format("{}", player.get_strength());
    }
    return std::format("18/{:02d}", exceptional % 100);
}

void display_basic_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    std::string line = std::format(
        "Name: {}   Class: {}   Race: {}   Level: {}",
        player.get_name(),
        player.playerClass,
        player.playerRace,
        player.get_level());
    ctx.renderer->draw_text(Vector2D{ x, panel_text_row_y(0, tileSize, row) }, line, WHITE_BLACK_PAIR);
    row += 2;
}

void display_experience_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    int currentXP = player.get_xp();
    int nextLevelXP = player.get_next_level_xp();
    int xpNeeded = nextLevelXP - currentXP;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) },
        std::format("XP: {} / {}   (Need: {})", currentXP, nextLevelXP, xpNeeded),
        CYAN_BLACK_PAIR);
    row += 2;
}

void display_attributes(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    const StrengthAttributes strengthRow = ctx.dataManager->strength_for(player.get_strength(), player.get_exceptional_strength());
    const int strHitMod = strengthRow.hitProb;
    const int strDmgMod = strengthRow.dmgAdj;
    const int conBonus = ctx.dataManager->constitution_for(player.get_constitution()).HPAdj;

    const DexterityAttributes dexterityRow = ctx.dataManager->dexterity_for(player.get_dexterity());
    const int missileAdj = dexterityRow.MissileAttackAdj;
    const int defensiveAdj = dexterityRow.DefensiveAdj;

    ctx.renderer->draw_text(Vector2D{ x, panel_text_row_y(0, tileSize, row) }, "--- ATTRIBUTES ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("STR: {:>2}  ({:+d} hit, {:+d} dmg)", strength_text(player), strHitMod, strDmgMod), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("DEX: {:2d}  ({:+d} missile, {:+d} defensive)", player.get_dexterity(), missileAdj, defensiveAdj), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("CON: {:2d}  ({:+d} HP/level)", player.get_constitution(), conBonus), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("INT: {:2d}   WIS: {:2d}   CHA: {:2d}", player.get_intelligence(), player.get_wisdom(), player.get_charisma()), WHITE_BLACK_PAIR);
    row += 2;
}

void display_combat_stats(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    int hp = player.get_hp();
    int maxHp = player.get_max_hp();
    int baseHP = player.get_hp_base();
    int conBonusTotal = maxHp - baseHP;

    ctx.renderer->draw_text(Vector2D{ x, panel_text_row_y(0, tileSize, row) }, "--- COMBAT ---", YELLOW_BLACK_PAIR);
    row++;

    int hpColor = (hp > maxHp / 2)
        ? GREEN_BLACK_PAIR
        : (hp > maxHp / 4 ? YELLOW_BLACK_PAIR : RED_BLACK_PAIR);

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("HP: {} / {}  (Base: {}, Con Bonus: {:+d})", hp, maxHp, baseHP, conBonusTotal), hpColor);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("THAC0: {}   AC: {}   DR: {}", player.get_thaco(), player.get_armor_class(), player.get_dr()), WHITE_BLACK_PAIR);
    row += 2;
}

void display_equipment_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    auto* equippedWeapon = player.get_equipped_item(EquipmentSlot::RIGHT_HAND);

    std::string damageDisplay;
    if (equippedWeapon && equippedWeapon->is_weapon())
    {
        const ItemEnhancement* enh = equippedWeapon->is_enhanced()
            ? &equippedWeapon->get_enhancement()
            : nullptr;
        damageDisplay = WeaponDamageRegistry::get_enhanced_damage_info(
            equippedWeapon->itemKey, enh)
                            .displayRoll;
    }
    else
    {
        damageDisplay = WeaponDamageRegistry::get_unarmed_damage_info().displayRoll;
    }

    const int strDmgMod = ctx.dataManager->strength_for(player.get_strength(), player.get_exceptional_strength()).dmgAdj;

    ctx.renderer->draw_text(Vector2D{ x, panel_text_row_y(0, tileSize, row) }, "--- EQUIPMENT ---", YELLOW_BLACK_PAIR);
    row++;

    std::string weaponName = equippedWeapon
        ? std::string(equippedWeapon->get_name())
        : "(unarmed)";

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("Weapon: {}   Damage: {}  (STR bonus: {:+d})", weaponName, damageDisplay, strDmgMod), WHITE_BLACK_PAIR);
    row += 2;
}

void display_right_panel_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int x = tileSize;

    std::string hungerStr = ctx.hungerSystem
        ? ctx.hungerSystem->get_hunger_state_string()
        : "Unknown";

    ctx.renderer->draw_text(Vector2D{ x, panel_text_row_y(0, tileSize, row) }, "--- OTHER ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, panel_text_row_y(0, tileSize, row) }, std::format("Gender: {}   Gold: {} gp   Hunger: {}", player.get_gender(), player.get_gold(), hungerStr), WHITE_BLACK_PAIR);
    row += 2;
}

} // anonymous namespace

// ============================================================================
// CharacterSheetUI
// ============================================================================

CharacterSheetUI::CharacterSheetUI(const Player& player)
    : player_ref(player)
{
}

void CharacterSheetUI::menu(GameContext& ctx)
{
    ctx.inputSystem->poll();
    GameKey key = ctx.inputSystem->get_key();
    if (key == GameKey::ESCAPE || key == GameKey::SPACE)
    {
        menu_set_run_false();
        return;
    }

    ctx.renderer->begin_frame();

    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    // The panel spans the screen in pixels. Whole tiles stop short of it at most
    // zooms, which left a strip unpainted and put the closing hint above the
    // panel's own floor, on top of the last row of content.
    int screenW = ctx.renderer->get_screen_width();
    int screenH = ctx.renderer->get_screen_height();

    ctx.renderer->draw_frame_pixels(Vector2D{ 0, 0 }, screenW, screenH, *ctx.tileConfig);

    std::string_view title = "CHARACTER SHEET";
    int title_w = ctx.renderer->measure_text(title);
    int title_x = (screenW - title_w) / 2;
    ctx.renderer->draw_text(Vector2D{ title_x, font_off }, title, YELLOW_BLACK_PAIR);

    std::string_view hint = "[ESC] or [SPACE] to close";
    int hint_w = ctx.renderer->measure_text(hint);
    int hint_x = (screenW - hint_w) / 2;
    ctx.renderer->draw_text(Vector2D{ hint_x, screenH - tileSize + font_off }, hint, CYAN_BLACK_PAIR);

    int row = 0;
    display_basic_info(player_ref, ctx, row);
    display_experience_info(player_ref, ctx, row);
    display_attributes(player_ref, ctx, row);
    display_combat_stats(player_ref, ctx, row);
    display_equipment_info(player_ref, ctx, row);
    display_right_panel_info(player_ref, ctx, row);

    ctx.renderer->end_frame();
}
