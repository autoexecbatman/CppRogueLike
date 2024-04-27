// file: RandomDice.h
#ifndef RANDOM_DICE_H
#define RANDOM_DICE_H

#include <random>

// This class is used to generate random numbers for our game.
// We want this class to be the only random number generator for the game right now.
class RandomDice
{
public:
	enum class DiceType : int
	{
		D2, D4, D6, D8, D10, D12, D20, D100
	} diceType{ DiceType::D6 };

	// public functions to emulate a set of dice from d2 to d100
	int d2() { diceType = DiceType::D2; return roll(1, 2); }
	int d4() { diceType = DiceType::D4; return roll(1, 4); }
	int d6() { diceType = DiceType::D6; return roll(1, 6); }
	int d8() { diceType = DiceType::D8; return roll(1, 8); }
	int d10() { diceType = DiceType::D10; return roll(1, 10); }
	int d12() { diceType = DiceType::D12; return roll(1, 12); }
	int d20() { diceType = DiceType::D20; return roll(1, 20); }
	int d100() { diceType = DiceType::D100; return roll(1, 100); }

	// get dice type
	std::string get_dice_type()
	{
		switch (diceType)
		{
		case DiceType::D2: return "D2";
		case DiceType::D4: return "D4";
		case DiceType::D6: return "D6";
		case DiceType::D8: return "D8";
		case DiceType::D10: return "D10";
		case DiceType::D12: return "D12";
		case DiceType::D20: return "D20";
		case DiceType::D100: return "D100";
		default: return "D6";
		}
	}

	// set dice using a string
	int roll_from_string(const std::string& diceType)
	{
		if (diceType == "D2") return d2();
		if (diceType == "D4") return d4();
		if (diceType == "D6") return d6();
		if (diceType == "D8") return d8();
		if (diceType == "D10") return d10();
		if (diceType == "D12") return d12();
		if (diceType == "D20") return d20();
		if (diceType == "D100") return d100();
		return d6();
	}

	int roll(int min, int max)
	{
		std::uniform_int_distribution<int> dist(min, max);
		return dist(m_gen);
	}

private:
	// we use the mersenne twister engine for our random number generator
	// we seed it with a random device
	std::mt19937 m_gen{ std::random_device{}() };

};

#endif // RANDOM_DICE_H
// end of file: RandomDice.h