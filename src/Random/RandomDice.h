// file: RandomDice.h
#ifndef RANDOM_DICE_H
#define RANDOM_DICE_H

#include <random>

// This class is used to generate random numbers for our game.
// We want this class to be the only random number generator for the game right now.
class RandomDice
{
public:
	// DEPRECATED: DiceType state tracking unused - scheduled for removal
	[[deprecated("Unused state tracking - scheduled for removal")]]
	enum class DiceType : int
	{
		D2, D4, D6, D8, D10, D12, D20, D100
	} diceType{ DiceType::D6 };

	// public functions to emulate a set of dice from d2 to d100
	int d2() { return roll(1, 2); }
	int d4() { return roll(1, 4); }
	int d6() { return roll(1, 6); }
	int d8() { return roll(1, 8); }
	int d10() { return roll(1, 10); }
	int d12() { return roll(1, 12); }
	int d20() { return roll(1, 20); }
	int d100() { return roll(1, 100); }

	// DEPRECATED: Unused state tracking - scheduled for removal
	[[deprecated("Unused - scheduled for removal")]]
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

	// Deprecated roll_from_string method removed - use DamageInfo::roll_damage() instead

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
