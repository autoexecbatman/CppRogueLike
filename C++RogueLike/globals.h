#ifndef PROJECT_PATH_GLOBALS_H_
#define PROJECT_PATH_GLOBALS_H_

#include <random>



//==DICE==
// make a global dice variables that randomise each time
// the dice is used for the damage of the player and the monsters
// the dice is used for the health of the player and the monsters
// the dice is used for everything
// a list of dice 
// example : 
// 1d4 , 1d6 , 1d8, 1d10, 1d12, 1d20, 1d100
class RandomDice
{
public:
	int d4() { int d4 = random_number_generator(1, 4); return d4; }
	int d6() { int d6 = random_number_generator(1, 6); return d6; }
	int d8() { int d8 = random_number_generator(1, 8); return d8; }
	int d10() { int d10 = random_number_generator(1, 10); return d10; }
	int d12() { int d12 = random_number_generator(1, 12); return d12; }
	int d20() { int d20 = random_number_generator(1, 20); return d20; }
	int d100() { int d100 = random_number_generator(1, 100); return d100; }

	int random_number_generator(int min, int max)
	{
		static std::random_device rd;
		static std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(min, max);

		return dist(mt);
	}
};

#endif // PROJECT_PATH_GLOBALS_H_