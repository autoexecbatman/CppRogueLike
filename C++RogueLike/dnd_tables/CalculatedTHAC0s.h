#pragma once

#include <vector>

struct CalculatedTHAC0s
{
	std::vector<int> cleric{ 0,20,20,20,18,18,18,16,16,16,14,14,14,12,12,12,10,10,10,8,8 };
	std::vector<int> rogue{ 0,20,20,19,19,18,18,17,17,16,16,15,15,14,14,13,13,12,12,11,11 };
	std::vector<int> warrior{ 0,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1 };
	std::vector<int> wizard{ 0,20,20,20,19,19,19,18,18,18,17,17,17,16,16,16,15,15,15,14,14 };

	int getFighter(int level)
	{
		if (level > 0) { return warrior.at(level); }
		else { game.err("Your level is less then 1."); return 1; }
	}
	int getRogue(int level)
	{
		if (level > 0) { return rogue.at(level); }
		else { game.err("Your level is less then 1."); return 1; }
	}
	int getCleric(int level)
	{
		if (level > 0) { return cleric.at(level); }
		else { game.err("Your level is less then 1."); return 1; }
	}
	int getWizard(int level)
	{
		if (level > 0) { return wizard.at(level); }
		else { game.err("Your level is less then 1."); return 1; }
	}
};