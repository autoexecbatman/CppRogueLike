#pragma once

#include <cassert>
#include <limits>
#include <random>
#include <vector>

// This class is used to generate random numbers for our game->
// We want this class to be the only random number generator for the game right now.
class RandomDice
{
public:
	RandomDice() = default;

	explicit RandomDice(unsigned int seed) noexcept
		: m_gen(seed)
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
		if (m_test_mode && !m_fixed_rolls.empty())
		{
			int value = m_fixed_rolls.front();
			m_fixed_rolls.erase(m_fixed_rolls.begin());
			if (value == HIGHEST)
			{
				return max;
			}
			// A queued value is the roll a test means a die to make, so it has to be one
			// that die can make. Scripting a 5 on a d4 asserts something about a
			// character no dice could produce, and it cannot be checked where the queue
			// is filled: what will be rolled against it is not known until something
			// rolls. Queue HIGHEST to mean the top of the range by name.
			assert(value >= min && value <= max && "a scripted roll is outside the range the die was asked for");
			return value;
		}
#endif
		std::uniform_int_distribution<int> dist(min, max);
		return dist(m_gen);
	}

#ifdef TESTING_MODE
	// Test-only methods for deterministic dice rolls
	void set_test_mode(bool enabled) { m_test_mode = enabled; }
	void set_next_d20(int value)
	{
		m_test_mode = true;
		m_fixed_rolls.push_back(value);
	}
	void set_next_roll(int value)
	{
		m_test_mode = true;
		m_fixed_rolls.push_back(value);
	}
	void clear_fixed_rolls() { m_fixed_rolls.clear(); }
#endif

	[[nodiscard]] std::mt19937& get_rng() noexcept { return m_gen; }

private:
	// we use the mersenne twister engine for our random number generator
	// we seed it with a random device
	std::mt19937 m_gen{ std::random_device{}() };

#ifdef TESTING_MODE
	bool m_test_mode{ false };
	std::vector<int> m_fixed_rolls;
#endif
};
