// Debug needs to be set to x86

#include <iostream>

#include "Engine.h"
#include "Colors.h"
#include "Window.h"
//==ENGINE==
// an instance of the Engine class.(singleton)
Engine engine(
	120, // int screenWidth
	30 // int screenHeight
);

int main()
{
	Colors colors;
	colors.my_init_pair();
	
	engine.game_menu(); // load the starting menu

	int countLoop = 0;
	while (engine.run == true)
	{
		//==DEBUG==
		std::clog << "Running...\nLoop number" << countLoop << std::endl;
		//==ENGINE_FUNCTIONS==
		engine.update(); // update map and actors positions
		engine.render(); // render map and actors to the screen
		//==INPUT==
		// get the input from the player
		engine.lastKey = engine.keyPress;
		engine.keyPress = getch();
		countLoop++;
	}
	engine.save();
	return 0;
}