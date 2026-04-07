#pragma once

#include <string>
#include <vector>
#include <memory>

#include "BaseMenu.h"

struct GameContext;

class DeathMenu : public BaseMenu
{
public:
    DeathMenu(GameContext& ctx);
    ~DeathMenu() override = default;
    DeathMenu(const DeathMenu&) = delete;
    DeathMenu& operator=(const DeathMenu&) = delete;
    DeathMenu(DeathMenu&&) = delete;
    DeathMenu& operator=(DeathMenu&&) = delete;

    void menu(GameContext& ctx) override;

private:
    void render(GameContext& ctx) const;
    void handle_input(GameContext& ctx);

    int dungeonLevel{};
    int playerLevel{};
    int playerXp{};
    int killCount{};
    std::string playerClass{};
    std::string playerRace{};
    std::vector<std::string> recentMessages{};
};
