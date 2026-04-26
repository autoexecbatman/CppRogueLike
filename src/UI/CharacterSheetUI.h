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
};
