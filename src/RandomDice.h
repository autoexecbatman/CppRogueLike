#pragma once

#include <cassert>
#include <limits>
#include <random>
#include <vector>

// This class is used to generate random numbers for our game->
// We want this class to be the only random number generator for the game right now.
class RandomDice
{
private:
	// we use the mersenne twister engine for our random number generator
	// we seed it with a random device
	std::mt19937 generator{ std::random_device{}() };

#ifdef TESTING_MODE
	bool testMode{ false };
	std::vector<int> fixedRolls;
#endif

public:
	RandomDice() = default;

	explicit RandomDice(unsigned int seed) noexcept
		: generator(seed)
	{
	}

	// public functions to emulate a set of dice from d2 to d100
	int d2() { return roll(1, 2); }

	int d4() { return roll(1, 4); }

	int d6() { return roll(1, 6); }

	int d8() { return roll(1, 8); }

	int d10() { return roll(1, 10); }

	int d12() { return roll(1, 12); }

	int d20() { return roll(1, 20); }

	int d100() { return roll(1, 100); }

	// Damage strings such as "1d8+2" are rolled by DamageInfo::roll_damage.

#ifdef TESTING_MODE
	// Queue this in place of a number to mean "whatever this die's highest is". A test
	// that wants a saving throw to fail does not always want to name the die it is
	// rolled on, and an out-of-range number meaning that cannot be told apart from
	// scripting a roll the die could never make. There is no LOWEST: nothing needed
	// one, and an unused name is a second thing to keep right.
	static constexpr int HIGHEST = std::numeric_limits<int>::max();
#endif

	int roll(int min, int max)
	{
#ifdef TESTING_MODE
		// In test mode, use fixed values if set
		if (testMode && !fixedRolls.empty())
		{
			int value = fixedRolls.front();
			fixedRolls.erase(fixedRolls.begin());
			if (value == HIGHEST)
			{
				return max;
			}
			// A queued value must be one this die can make; the range is known only now, when something rolls.
			// Queue HIGHEST to mean the top of the range by name.
			assert(value >= min && value <= max && "a scripted roll is outside the range the die was asked for");
			return value;
		}
#endif
		std::uniform_int_distribution<int> dist(min, max);
		return dist(generator);
	}

#ifdef TESTING_MODE
	// Test-only methods for deterministic dice rolls
	void set_test_mode(bool enabled) { testMode = enabled; }

	void set_next_d20(int value)
	{
		testMode = true;
		fixedRolls.push_back(value);
	}

	void set_next_roll(int value)
	{
		testMode = true;
		fixedRolls.push_back(value);
	}

	void clear_fixed_rolls() { fixedRolls.clear(); }
#endif

	[[nodiscard]] std::mt19937& get_rng() noexcept { return generator; }
};
