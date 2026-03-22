#pragma once

#include "../Menu/BaseMenu.h"

class Player;
struct GameContext;

class CharacterSheetUI : public BaseMenu
{
public:
	CharacterSheetUI(const Player& player, GameContext& ctx);
	~CharacterSheetUI() = default;
	CharacterSheetUI(const CharacterSheetUI&) = delete;
	CharacterSheetUI& operator=(const CharacterSheetUI&) = delete;
	CharacterSheetUI(CharacterSheetUI&&) = delete;
	CharacterSheetUI& operator=(CharacterSheetUI&&) = delete;

	void menu(GameContext& ctx) override;

private:
	const Player& player_ref;

	static void display_basic_info(const Player& player, GameContext& ctx, int& row);
	static void display_experience_info(const Player& player, GameContext& ctx, int& row);
	static void display_attributes(const Player& player, GameContext& ctx, int& row);
	static void display_combat_stats(const Player& player, GameContext& ctx, int& row);
	static void display_equipment_info(const Player& player, GameContext& ctx, int& row);
	static void display_right_panel_info(const Player& player, GameContext& ctx, int& row);

	static int get_strength_hit_modifier(const Player& player, GameContext& ctx);
	static int get_strength_damage_modifier(const Player& player, GameContext& ctx);
	static int get_constitution_bonus(const Player& player, GameContext& ctx);
};
