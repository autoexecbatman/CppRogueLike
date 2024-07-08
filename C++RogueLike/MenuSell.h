#pragma once

#include "BaseMenu.h"
#include "Actor/Actor.h"

class MenuSell : public BaseMenu
{
	Creature& player;
public:
	MenuSell(Creature& player);
	~MenuSell();

	void draw();
	void on_key(int key);
	void menu() override;
};