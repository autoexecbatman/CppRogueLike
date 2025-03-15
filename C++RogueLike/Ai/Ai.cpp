// file: Ai.cpp
#include <iostream>
#include <gsl/util>
#include <libtcod.h>

#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number()) {
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<AiType>(j["type"].get<int>());
	std::unique_ptr<Ai> ai;

	switch (type) {
	case AiType::PLAYER:
		ai = std::make_unique<AiPlayer>();
		break;
	case AiType::MONSTER:
		ai = std::make_unique<AiMonster>();
		break;
	case AiType::CONFUSED_MONSTER:
		ai = std::make_unique<AiMonsterConfused>(0,nullptr);
		break;
	case AiType::SHOPKEEPER:
		ai = std::make_unique<AiShopkeeper>();
		break;
	default:
		throw std::runtime_error("Unknown AiType");
	} // end of switch (type)

	ai->load(j);
	return ai;
}

// If positionDifference > 0, return 1; otherwise, return -1
int Ai::calculate_step(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}
//====

// end of file: Ai.cpp
