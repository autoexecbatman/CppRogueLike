// file: RandomDice.h
#ifndef RANDOM_DICE_H
#define RANDOM_DICE_H

#include <random>

class RandomDice
{
public:
    int d4() { return roll(4); }
    int d6() { return roll(6); }
    int d8() { return roll(8); }
    int d10() { return roll(10); }
    int d12() { return roll(12); }
    int d20() { return roll(20); }
    int d100() { return roll(100); }

private:
    std::mt19937 m_gen{ std::random_device{}() };

    int roll(int max) {
        return m_gen() % max + 1;
    }
};

#endif // RANDOM_DICE_H
// end of file: RandomDice.h