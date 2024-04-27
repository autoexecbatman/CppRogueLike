#pragma once

#include <vector>

struct CharismaAttributes
{
	int Cha{};
	int MaxHencmen{};
	int Loyalty{};
	int ReactionAdj{};

	void print_chart();
};

std::vector<CharismaAttributes> loadCharismaAttributes();