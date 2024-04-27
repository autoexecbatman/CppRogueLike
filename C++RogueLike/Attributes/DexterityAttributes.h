#pragma once

#include <vector>

struct DexterityAttributes
{
	int Dex{};
	int ReactionAdj{};
	int MissileAttackAdj{};
	int DefensiveAdj{};
};

std::vector<DexterityAttributes> loadDexterityAttributes();