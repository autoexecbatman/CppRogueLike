// CharacterSheetUI.cpp - Handles character sheet display

#include "CharacterSheetUI.h"
#include "../ActorTypes/Player.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include <format>

void CharacterSheetUI::display_character_sheet(const Player& player)
{
    WINDOW* character_sheet = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_Y, WINDOW_X);
    box(character_sheet, 0, 0);
    refresh();

    bool run = true;
    while (run)
    {
        // Clear the window content (but keep the border)
        for (int y = 1; y < WINDOW_HEIGHT - 1; y++)
        {
            for (int x = 1; x < WINDOW_WIDTH - 1; x++)
            {
                mvwaddch(character_sheet, y, x, ' ');
            }
        }

        // Display all sections
        display_basic_info(character_sheet, player);
        display_experience_info(character_sheet, player);
        display_attributes(character_sheet, player);
        display_combat_stats(character_sheet, player);
        display_equipment_info(character_sheet, player);
        display_right_panel_info(character_sheet, player);
        display_constitution_effects(character_sheet, player);
        display_strength_effects(character_sheet, player);

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
    mvwprintw(window, 4, 1, "Level: %d", player.playerLevel);
}

void CharacterSheetUI::display_experience_info(WINDOW* window, const Player& player)
{
    // Calculate XP needed for next level
    int currentXP = player.destructible->get_xp();
    int nextLevelXP = player.ai->get_next_level_xp(const_cast<Player&>(player));
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
        player.playerLevel + 1
    );
    mvwprintw(window, 6, 1, "XP needed for next level: %d", xpNeeded);
}

void CharacterSheetUI::display_attributes(WINDOW* window, const Player& player)
{
    mvwprintw(window, 8, 1, "Attributes:");

    // Get strength modifiers from attributes table
    int strHitMod = get_strength_hit_modifier(player);
    int strDmgMod = get_strength_damage_modifier(player);

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
    int conBonus = get_constitution_bonus(player);

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

void CharacterSheetUI::display_combat_stats(WINDOW* window, const Player& player)
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
    int strHitMod = get_strength_hit_modifier(player);
    int strDmgMod = get_strength_damage_modifier(player);
    
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
    const auto& dexterityAttributes = game.data_manager.get_dexterity_attributes();
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

void CharacterSheetUI::display_equipment_info(WINDOW* window, const Player& player)
{
	// Equipped weapon info with color
	wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
	mvwprintw(window, 24, 1, "Equipment:");

	// Get equipped weapon from proper slot-based equipment system
	auto* equippedWeapon = player.get_equipped_item(EquipmentSlot::RIGHT_HAND);
	if (equippedWeapon)
	{
		mvwprintw(
			window,
			25,
			3,
			"Weapon: %s (%s damage)",
			equippedWeapon->actorData.name.c_str(),
			player.attacker->get_roll().c_str()
		);

		// Show if weapon is ranged
		if (player.has_state(ActorState::IS_RANGED))
		{
			wattron(window, COLOR_PAIR(WHITE_BLUE_PAIR));
			mvwprintw(window, 25, 40, "[Ranged]");
			wattroff(window, COLOR_PAIR(WHITE_BLUE_PAIR));
		}

		// Show effective damage with strength bonus
		int strDmgMod = get_strength_damage_modifier(player);
		if (strDmgMod != 0)
		{
			wattron(window, COLOR_PAIR(RED_YELLOW_PAIR));
			mvwprintw(
				window,
				26,
				3,
				"Effective damage: %s %+d",
				player.attacker->get_roll().c_str(),
				strDmgMod
			);
			wattroff(window, COLOR_PAIR(RED_YELLOW_PAIR));
		}
	}
	else
	{
		mvwprintw(window, 25, 3, "Weapon: Unarmed (%s damage)", player.attacker->get_roll().c_str());

		// Show effective unarmed damage with strength bonus
		int strDmgMod = get_strength_damage_modifier(player);
		if (strDmgMod != 0)
		{
			wattron(window, COLOR_PAIR(RED_YELLOW_PAIR));
			mvwprintw(window, 26, 3, "Effective damage: %s %+d", player.attacker->get_roll().c_str(), strDmgMod);
			wattroff(window, COLOR_PAIR(RED_YELLOW_PAIR));
		}
	}
	wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
}

void CharacterSheetUI::display_right_panel_info(WINDOW* window, const Player& player)
{
    // Add gold and other stats on the right side
    mvwprintw(window, 9, 60, "Gender: %s", player.get_gender().c_str());
    mvwprintw(window, 10, 60, "Gold: %d", player.get_gold());
    
    // Enhanced hunger display with numbers and bar
    wattron(window, COLOR_PAIR(game.hunger_system.get_hunger_color()));
    mvwprintw(
        window,
        11,
        60,
        "Hunger: %s (%s)",
        game.hunger_system.get_hunger_numerical_string().c_str(),
        game.hunger_system.get_hunger_state_string().c_str()
    );
    wattroff(window, COLOR_PAIR(game.hunger_system.get_hunger_color()));
    
    // Display hunger bar on next line
    wattron(window, COLOR_PAIR(game.hunger_system.get_hunger_color()));
    mvwprintw(
        window,
        12,
        60,
        "%s",
        game.hunger_system.get_hunger_bar_string(15).c_str()
    );
    wattroff(window, COLOR_PAIR(game.hunger_system.get_hunger_color()));
}

void CharacterSheetUI::display_constitution_effects(WINDOW* window, const Player& player)
{
    // Add Constitution details panel on the right side
    mvwprintw(window, 14, 60, "Constitution Effects:");

    if (player.get_constitution() >= 1 && player.get_constitution() <= game.data_manager.get_constitution_attributes().size())
    {
        const auto& conAttr = game.data_manager.get_constitution_attributes().at(player.get_constitution() - 1);

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

void CharacterSheetUI::display_strength_effects(WINDOW* window, const Player& player)
{
    // Add strength details panel on the right side
    mvwprintw(window, 21, 60, "Strength Effects:");

    if (player.get_strength() >= 1 && player.get_strength() <= game.data_manager.get_strength_attributes().size())
    {
        const auto& strAttr = game.data_manager.get_strength_attributes().at(player.get_strength() - 1);

        mvwprintw(window, 22, 62, "Hit Probability Adj: %+d", strAttr.hitProb);
        mvwprintw(window, 23, 62, "Damage Adjustment: %+d", strAttr.dmgAdj);
        mvwprintw(window, 24, 62, "Weight Allowance: %d lbs", strAttr.wgtAllow);
        mvwprintw(window, 25, 62, "Max Press: %d lbs", strAttr.maxPress);
        mvwprintw(window, 26, 62, "Open Doors: %d/6", strAttr.openDoors);
    }
}

int CharacterSheetUI::get_strength_hit_modifier(const Player& player)
{
    if (player.get_strength() > 0 && player.get_strength() <= game.data_manager.get_strength_attributes().size())
    {
        return game.data_manager.get_strength_attributes().at(player.get_strength() - 1).hitProb;
    }
    return 0;
}

int CharacterSheetUI::get_strength_damage_modifier(const Player& player)
{
    if (player.get_strength() > 0 && player.get_strength() <= game.data_manager.get_strength_attributes().size())
    {
        return game.data_manager.get_strength_attributes().at(player.get_strength() - 1).dmgAdj;
    }
    return 0;
}

int CharacterSheetUI::get_constitution_bonus(const Player& player)
{
    if (player.get_constitution() >= 1 && player.get_constitution() <= game.data_manager.get_constitution_attributes().size())
    {
        return game.data_manager.get_constitution_attributes().at(player.get_constitution() - 1).HPAdj;
    }
    return 0;
}

void CharacterSheetUI::cleanup_and_restore()
{
    // Redraw screen
    clear();
    refresh();
}
