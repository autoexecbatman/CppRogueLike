#include <iostream>
#include "IMenuOptions.h"

// if new game is selected run next menu (gender)
void NewGame::on_selection()
{
	std::cout << "New Game Selected" << std::endl;

	// set menu run flag to false to halt the while loop
	auto m = dynamic_cast<Menu*>(game.menus.front().get());
	m->run = false;
	game.deadMenus.push_back(std::move(game.menus.front()));
	game.menus.push_back(std::make_unique<MenuGender>());
}
	/*game.menus.pop_front();*/ // can't pop front here because it is still running

	// remove current menu from the que
	/*game.menus.pop_front();*/

	/*MenuGender menuGender;*/
	/*menuGender.menu_gender();*/
	// if back is selected, set run to false
	//if (menuGender.currentGenderOption == MenuGender::MenuGenderOptions::BACK)
	//{
	//	menuGender.run = false;
	//}
	//else
	//{
	//	game.init();
	//}
//}