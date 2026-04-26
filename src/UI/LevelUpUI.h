#pragma once

#include "../Menu/BaseMenu.h"

class Player;
struct GameContext;

class LevelUpUI : public BaseMenu
{
public:
    LevelUpUI(Player& player, int newLevel, GameContext& ctx);
    ~LevelUpUI() = default;
    LevelUpUI(const LevelUpUI&) = delete;
    LevelUpUI& operator=(const LevelUpUI&) = delete;
    LevelUpUI(LevelUpUI&&) = delete;
    LevelUpUI& operator=(LevelUpUI&&) = delete;

    void menu(GameContext& ctx) override;

private:
    Player& player_ref;
    int level;
};
