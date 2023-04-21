// file: RandomDice.h
#ifndef RANDOM_DICE_H
#define RANDOM_DICE_H

#include <random>

class RandomDice
{
public:
	int d4() { const int d4 = random_number_generator(1, 4); return d4; }
	int d6() { const int d6 = random_number_generator(1, 6); return d6; }
	int d8() { const int d8 = random_number_generator(1, 8); return d8; }
	int d10() { const int d10 = random_number_generator(1, 10); return d10; }
	int d12() { const int d12 = random_number_generator(1, 12); return d12; }
	int d20() { const int d20 = random_number_generator(1, 20); return d20; }
	int d100() { const int d100 = random_number_generator(1, 100); return d100; }

	int random_number_generator(int min, int max)
	{
		static std::random_device rd;
		static std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(min, max);

		return dist(mt);
	}
};

#endif // RANDOM_DICE_H
// end of file: globals.h
