#pragma once

#include "BaseMenu.h"

class Player;
struct GameContext;

class LevelUpUI : public BaseMenu
{
private:
	Player& playerRef;
	int level{};

public:
	LevelUpUI(Player& player, int newLevel);

	void menu(GameContext& ctx) override;
};
