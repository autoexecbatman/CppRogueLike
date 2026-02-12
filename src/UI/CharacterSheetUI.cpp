// CharacterSheetUI.cpp - Handles character sheet display
#include <format>
#include <curses.h>

#include "CharacterSheetUI.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Combat/WeaponDamageRegistry.h"

void CharacterSheetUI::display_character_sheet(const Player& player, GameContext& ctx)
{
    WINDOW* character_sheet = newwin(window_height(), window_width(), WINDOW_Y, WINDOW_X);
    box(character_sheet, 0, 0);
    refresh();

    bool run = true;
    while (run)
    {
        // Clear the window content (but keep the border)
        for (int y = 1; y < window_height() - 1; y++)
        {
            for (int x = 1; x < window_width() - 1; x++)
            {
                mvwaddch(character_sheet, y, x, ' ');
            }
        }

        // Display all sections
        display_basic_info(character_sheet, player);
        display_experience_info(character_sheet, player, ctx);
        display_attributes(character_sheet, player, ctx);
        display_combat_stats(character_sheet, player, ctx);
        display_equipment_info(character_sheet, player, ctx);
        display_right_panel_info(character_sheet, player, ctx);
        display_constitution_effects(character_sheet, player, ctx);
        display_strength_effects(character_sheet, player, ctx);

        mvwprintw(character_sheet, 28, 1, "Press any key to close...");
        wrefresh(character_sheet);

        const int key = getch();
        if (key != ERR) {
            run = false;
        }
    }

    delwin(character_sheet);
    cleanup_and_restore();
}

void CharacterSheetUI::display_basic_info(WINDOW* window, const Player& player)
{
    mvwprintw(window, 1, 1, "Name: %s", player.actorData.name.c_str());
    mvwprintw(window, 2, 1, "Class: %s", player.playerClass.c_str());
    mvwprintw(window, 3, 1, "Race: %s", player.playerRace.c_str());
    mvwprintw(window, 4, 1, "Level: %d", player.get_player_level());
}

void CharacterSheetUI::display_experience_info(WINDOW* window, const Player& player, GameContext& ctx)
{
    // Calculate XP needed for next level
    int currentXP = player.destructible->get_xp();
    int nextLevelXP = player.ai->get_next_level_xp(ctx, const_cast<Player&>(player));
    int xpNeeded = nextLevelXP - currentXP;
    float progressPercent = static_cast<float>(currentXP) / static_cast<float>(nextLevelXP) * 100.0f;

    // Enhanced XP display with progress to next level
    mvwprintw(
        window,
        5,
        1,
        "Experience: %d / %d (%.1f%% to level %d)",
        currentXP,
        nextLevelXP,
        progressPercent,
        player.get_player_level() + 1
    );
    mvwprintw(window, 6, 1, "XP needed for next level: %d", xpNeeded);
}

void CharacterSheetUI::display_attributes(WINDOW* window, const Player& player, GameContext& ctx)
{
    mvwprintw(window, 8, 1, "Attributes:");

    // Get strength modifiers from attributes table
    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);

    // Display Strength with modifiers
    wattron(window, A_BOLD);
    mvwprintw(window, 9, 3, "Strength: %d", player.get_strength());
    wattroff(window, A_BOLD);

    // Show hit and damage modifiers from strength
    if (strHitMod != 0 || strDmgMod != 0)
    {
        wattron(window, COLOR_PAIR((strHitMod >= 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        mvwprintw(window, 9, 20, "To Hit: %+d", strHitMod);
        wattroff(window, COLOR_PAIR((strHitMod >= 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));

        wattron(window, COLOR_PAIR((strDmgMod >= 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        mvwprintw(window, 9, 35, "Damage: %+d", strDmgMod);
        wattroff(window, COLOR_PAIR((strDmgMod >= 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
    }

    // Display Constitution with HP bonus effect
    int conBonus = get_constitution_bonus(player, ctx);

    mvwprintw(window, 10, 3, "Dexterity: %d", player.get_dexterity());

    if (conBonus != 0)
    {
        wattron(window, COLOR_PAIR((conBonus > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        mvwprintw(
            window,
            11,
            3,
            "Constitution: %d (%+d HP per level)",
            player.get_constitution(),
            conBonus
        );
        wattroff(window, COLOR_PAIR((conBonus > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
    }
    else
    {
        mvwprintw(window, 11, 3, "Constitution: %d", player.get_constitution());
    }

    mvwprintw(window, 12, 3, "Intelligence: %d", player.get_intelligence());
    mvwprintw(window, 13, 3, "Wisdom: %d", player.get_wisdom());
    mvwprintw(window, 14, 3, "Charisma: %d", player.get_charisma());
}

void CharacterSheetUI::display_combat_stats(WINDOW* window, const Player& player, GameContext& ctx)
{
    mvwprintw(window, 16, 1, "Combat Statistics:");

    // Calculate base HP and Con bonus
    int baseHP = player.destructible->get_hp_base();
    int conBonusTotal = player.destructible->get_max_hp() - baseHP;

    // Show HP with Constitution effect
    if (conBonusTotal != 0)
    {
        mvwprintw(
            window,
            17,
            3,
            "HP: %d/%d ",
            player.destructible->get_hp(),
            player.destructible->get_max_hp()
        );

        wattron(window, COLOR_PAIR((conBonusTotal > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        wprintw(
            window,
            "(%d base %+d Con bonus)",
            baseHP,
            conBonusTotal
        );
        wattroff(window, COLOR_PAIR((conBonusTotal > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
    }
    else
    {
        mvwprintw(
            window,
            17,
            3,
            "HP: %d/%d",
            player.destructible->get_hp(),
            player.destructible->get_max_hp()
        );
    }

    // Display attack bonus from strength if applicable
    int strHitMod = get_strength_hit_modifier(player, ctx);
    int strDmgMod = get_strength_damage_modifier(player, ctx);
    
    if (strHitMod != 0 || strDmgMod != 0) {
        mvwprintw(window, 18, 3, "Attack Bonuses: ");
        if (strHitMod != 0)
        {
            wattron(window, COLOR_PAIR((strHitMod > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
            wprintw(window, "%+d to hit ", strHitMod);
            wattroff(window, COLOR_PAIR((strHitMod > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        }
        if (strDmgMod != 0)
        {
            wattron(window, COLOR_PAIR((strDmgMod > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
            wprintw(window, "%+d to damage", strDmgMod);
            wattroff(window, COLOR_PAIR((strDmgMod > 0) ? WHITE_GREEN_PAIR : WHITE_RED_PAIR));
        }
    }

    mvwprintw(window, 19, 3, "Armor Class: %d", player.destructible->get_armor_class());
    mvwprintw(window, 20, 3, "THAC0: %d", player.destructible->get_thaco());

    // Display dexterity bonuses
    const auto& dexterityAttributes = ctx.data_manager->get_dexterity_attributes();
    if (player.get_dexterity() > 0 && player.get_dexterity() <= dexterityAttributes.size())
    {
        // Show missile attack adjustment
        int missileAdj = dexterityAttributes.at(player.get_dexterity() - 1).MissileAttackAdj;
        mvwprintw(window, 21, 3, "Ranged Attack Bonus: %d", missileAdj);

        // Show defensive adjustment
        int defensiveAdj = dexterityAttributes.at(player.get_dexterity() - 1).DefensiveAdj;
        mvwprintw(window, 22, 3, "Defensive Adjustment: %d AC", defensiveAdj);
    }
}

void CharacterSheetUI::display_equipment_info(WINDOW* window, const Player& player, GameContext& ctx)
{
	// Equipped weapon info with color
	wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
	mvwprintw(window, 24, 1, "Equipment:");

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

	if (equippedWeapon)
	{
		mvwprintw(window, 25, 3, "Weapon: %s (%s damage)", equippedWeapon->actorData.name.c_str(), damageDisplay.c_str());

		if (player.has_state(ActorState::IS_RANGED))
		{
			wattron(window, COLOR_PAIR(WHITE_BLUE_PAIR));
			mvwprintw(window, 25, 40, "[Ranged]");
			wattroff(window, COLOR_PAIR(WHITE_BLUE_PAIR));
		}

		int strDmgMod = get_strength_damage_modifier(player, ctx);
		if (strDmgMod != 0)
		{
			wattron(window, COLOR_PAIR(RED_YELLOW_PAIR));
			mvwprintw(window, 26, 3, "Effective damage: %s %+d", damageDisplay.c_str(), strDmgMod);
			wattroff(window, COLOR_PAIR(RED_YELLOW_PAIR));
		}
	}
	else
	{
		mvwprintw(window, 25, 3, "Weapon: Unarmed (%s damage)", damageDisplay.c_str());

		int strDmgMod = get_strength_damage_modifier(player, ctx);
		if (strDmgMod != 0)
		{
			wattron(window, COLOR_PAIR(RED_YELLOW_PAIR));
			mvwprintw(window, 26, 3, "Effective damage: %s %+d", damageDisplay.c_str(), strDmgMod);
			wattroff(window, COLOR_PAIR(RED_YELLOW_PAIR));
		}
	}
	wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
}

void CharacterSheetUI::display_right_panel_info(WINDOW* window, const Player& player, GameContext& ctx)
{
    // Add gold and other stats on the right side
    mvwprintw(window, 9, 60, "Gender: %s", player.get_gender().c_str());
    mvwprintw(window, 10, 60, "Gold: %d", player.get_gold());
    
    // Enhanced hunger display with numbers and bar
    wattron(window, COLOR_PAIR(ctx.hunger_system->get_hunger_color()));
    mvwprintw(
        window,
        11,
        60,
        "Hunger: %s (%s)",
        ctx.hunger_system->get_hunger_numerical_string().c_str(),
        ctx.hunger_system->get_hunger_state_string().c_str()
    );
    wattroff(window, COLOR_PAIR(ctx.hunger_system->get_hunger_color()));
    
    // Display hunger bar on next line
    wattron(window, COLOR_PAIR(ctx.hunger_system->get_hunger_color()));
    mvwprintw(
        window,
        12,
        60,
        "%s",
        ctx.hunger_system->get_hunger_bar_string(15).c_str()
    );
    wattroff(window, COLOR_PAIR(ctx.hunger_system->get_hunger_color()));
}

void CharacterSheetUI::display_constitution_effects(WINDOW* window, const Player& player, GameContext& ctx)
{
    // Add Constitution details panel on the right side
    mvwprintw(window, 14, 60, "Constitution Effects:");

    if (player.get_constitution() >= 1 && player.get_constitution() <= ctx.data_manager->get_constitution_attributes().size())
    {
        const auto& conAttr = ctx.data_manager->get_constitution_attributes().at(player.get_constitution() - 1);

        mvwprintw(window, 15, 62, "HP Adjustment: %+d per level", conAttr.HPAdj);
        mvwprintw(window, 16, 62, "System Shock: %d%%", conAttr.SystemShock);
        mvwprintw(window, 17, 62, "Resurrection Survival: %d%%", conAttr.ResurrectionSurvival);
        mvwprintw(window, 18, 62, "Poison Save Modifier: %+d", conAttr.PoisonSave);

        if (conAttr.Regeneration > 0)
        {
            mvwprintw(window, 19, 62, "Regeneration: %d HP per turn", conAttr.Regeneration);
        }
    }
}

void CharacterSheetUI::display_strength_effects(WINDOW* window, const Player& player, GameContext& ctx)
{
    // Add strength details panel on the right side
    mvwprintw(window, 21, 60, "Strength Effects:");

    if (player.get_strength() >= 1 && player.get_strength() <= ctx.data_manager->get_strength_attributes().size())
    {
        const auto& strAttr = ctx.data_manager->get_strength_attributes().at(player.get_strength() - 1);

        mvwprintw(window, 22, 62, "Hit Probability Adj: %+d", strAttr.hitProb);
        mvwprintw(window, 23, 62, "Damage Adjustment: %+d", strAttr.dmgAdj);
        mvwprintw(window, 24, 62, "Weight Allowance: %d lbs", strAttr.wgtAllow);
        mvwprintw(window, 25, 62, "Max Press: %d lbs", strAttr.maxPress);
        mvwprintw(window, 26, 62, "Open Doors: %d/6", strAttr.openDoors);
    }
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
    // Redraw screen
    clear();
    refresh();
}
