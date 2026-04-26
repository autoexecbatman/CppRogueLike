// CharacterSheetUI.cpp - Handles character sheet display
#include <format>
#include <string>

#include <raylib.h>

#include "../Actor/EquipmentSlot.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "CharacterSheetUI.h"

// ============================================================================
// Private helpers — not visible outside this translation unit.
// ============================================================================

namespace
{

int get_strength_hit_modifier(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.dataManager->get_strength_attributes();
    if (player.get_strength() > 0 &&
        player.get_strength() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_strength() - 1).hitProb;
    }
    return 0;
}

int get_strength_damage_modifier(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.dataManager->get_strength_attributes();
    if (player.get_strength() > 0 &&
        player.get_strength() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_strength() - 1).dmgAdj;
    }
    return 0;
}

int get_constitution_bonus(const Player& player, GameContext& ctx)
{
    const auto& attrs = ctx.dataManager->get_constitution_attributes();
    if (player.get_constitution() >= 1 &&
        player.get_constitution() <= static_cast<int>(attrs.size()))
    {
        return attrs.at(player.get_constitution() - 1).HPAdj;
    }
    return 0;
}

void display_basic_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    std::string line = std::format(
        "Name: {}   Class: {}   Race: {}   Level: {}",
        player.get_name(),
        player.playerClass,
        player.playerRace,
        player.get_level());
    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + font_off }, line, WHITE_BLACK_PAIR);
    row += 2;
}

void display_experience_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    int currentXP = player.get_xp();
    int nextLevelXP = player.get_next_level_xp(ctx);
    int xpNeeded = nextLevelXP - currentXP;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off },
        std::format("XP: {} / {}   (Need: {})", currentXP, nextLevelXP, xpNeeded),
        CYAN_BLACK_PAIR);
    row += 2;
}

void display_attributes(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);
    int conBonus = get_constitution_bonus(player, ctx);

    int missileAdj = 0;
    int defensiveAdj = 0;
    const auto& dexAttr = ctx.dataManager->get_dexterity_attributes();
    if (player.get_dexterity() > 0 &&
        player.get_dexterity() <= static_cast<int>(dexAttr.size()))
    {
        missileAdj = dexAttr.at(player.get_dexterity() - 1).MissileAttackAdj;
        defensiveAdj = dexAttr.at(player.get_dexterity() - 1).DefensiveAdj;
    }

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + font_off }, "--- ATTRIBUTES ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("STR: {:2d}  ({:+d} hit, {:+d} dmg)", player.get_strength(), strHitMod, strDmgMod), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("DEX: {:2d}  ({:+d} missile, {:+d} defensive)", player.get_dexterity(), missileAdj, defensiveAdj), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("CON: {:2d}  ({:+d} HP/level)", player.get_constitution(), conBonus), WHITE_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("INT: {:2d}   WIS: {:2d}   CHA: {:2d}", player.get_intelligence(), player.get_wisdom(), player.get_charisma()), WHITE_BLACK_PAIR);
    row += 2;
}

void display_combat_stats(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    int hp = player.destructible->get_hp();
    int maxHp = player.destructible->get_max_hp();
    int baseHP = player.destructible->get_hp_base();
    int conBonusTotal = maxHp - baseHP;

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + font_off }, "--- COMBAT ---", YELLOW_BLACK_PAIR);
    row++;

    int hpColor = (hp > maxHp / 2)
        ? GREEN_BLACK_PAIR
        : (hp > maxHp / 4 ? YELLOW_BLACK_PAIR : RED_BLACK_PAIR);

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("HP: {} / {}  (Base: {}, Con Bonus: {:+d})", hp, maxHp, baseHP, conBonusTotal), hpColor);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("THAC0: {}   AC: {}   DR: {}", player.get_thaco(), player.get_armor_class(), player.get_dr()), WHITE_BLACK_PAIR);
    row += 2;
}

void display_equipment_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    auto* equippedWeapon = player.get_equipped_item(EquipmentSlot::RIGHT_HAND);

    std::string damageDisplay;
    if (equippedWeapon && equippedWeapon->is_weapon())
    {
        const ItemEnhancement* enh = equippedWeapon->is_enhanced()
            ? &equippedWeapon->get_enhancement()
            : nullptr;
        damageDisplay = WeaponDamageRegistry::get_enhanced_damage_info(
            equippedWeapon->item_key, enh)
                            .displayRoll;
    }
    else
    {
        damageDisplay = WeaponDamageRegistry::get_unarmed_damage_info().displayRoll;
    }

    int strDmgMod = get_strength_damage_modifier(player, ctx);

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + font_off }, "--- EQUIPMENT ---", YELLOW_BLACK_PAIR);
    row++;

    std::string weaponName = equippedWeapon
        ? std::string(equippedWeapon->get_name())
        : "(unarmed)";

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("Weapon: {}   Damage: {}  (STR bonus: {:+d})", weaponName, damageDisplay, strDmgMod), WHITE_BLACK_PAIR);
    row += 2;
}

void display_right_panel_info(const Player& player, GameContext& ctx, int& row)
{
    int tileSize = ctx.renderer->get_tile_size();
    int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
    int x = tileSize;

    std::string hungerStr = ctx.hungerSystem
        ? ctx.hungerSystem->get_hunger_state_string()
        : "Unknown";

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + font_off }, "--- OTHER ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        Vector2D{ x, row * tileSize + font_off }, std::format("Gender: {}   Gold: {} gp   Hunger: {}", player.get_gender(), player.get_gold(), hungerStr), WHITE_BLACK_PAIR);
    row += 2;
}

} // anonymous namespace

// ============================================================================
// CharacterSheetUI
// ============================================================================

CharacterSheetUI::CharacterSheetUI(const Player& player, GameContext& /*ctx*/)
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
    int vcols = ctx.renderer->get_viewport_cols();
    int vrows = ctx.renderer->get_viewport_rows();

    ctx.renderer->draw_frame(Vector2D{ 0, 0 }, vcols, vrows, *ctx.tileConfig);

    std::string_view title = "CHARACTER SHEET";
    int title_w = ctx.renderer->measure_text(title);
    int title_x = (vcols * tileSize - title_w) / 2;
    ctx.renderer->draw_text(Vector2D{ title_x, font_off }, title, YELLOW_BLACK_PAIR);

    std::string_view hint = "[ESC] or [SPACE] to close";
    int hint_w = ctx.renderer->measure_text(hint);
    int hint_x = (vcols * tileSize - hint_w) / 2;
    ctx.renderer->draw_text(Vector2D{ hint_x, (vrows - 1) * tileSize + font_off }, hint, CYAN_BLACK_PAIR);

    int row = 1;
    display_basic_info(player_ref, ctx, row);
    display_experience_info(player_ref, ctx, row);
    display_attributes(player_ref, ctx, row);
    display_combat_stats(player_ref, ctx, row);
    display_equipment_info(player_ref, ctx, row);
    display_right_panel_info(player_ref, ctx, row);

    ctx.renderer->end_frame();
}
