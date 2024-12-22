// file: Ai.cpp
#include <iostream>
#include <gsl/util>
#include <libtcod.h>

#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"

//==AI==
std::unique_ptr<Ai> Ai::create(TCODZip& zip)
{
	// read the type of the ai
	const AiType type{ gsl::narrow_cast<AiType>(zip.getInt()) };
	std::unique_ptr<Ai> ai{};

	switch (type)
	{
	case AiType::PLAYER:
		ai = std::make_unique<AiPlayer>();
		break;
	case AiType::MONSTER:
		ai = std::make_unique<AiMonster>();
		break;
	case AiType::CONFUSED_MONSTER:
		ai = std::make_unique<AiMonsterConfused>(0, std::make_unique<AiMonster>());
		break;
	case AiType::SHOPKEEPER:
		ai = std::make_unique<AiShopkeeper>();
		break;
	default:
		std::cout << "Error: Ai::create() - unknown AiType" << std::endl;
		ai = std::make_unique<AiPlayer>(); // assign default dummy value
		break;
	}

	if (ai)
	{
		try
		{
			ai->load(zip);
		}
		catch (const std::exception& e)
		{
			std::cout << "Error: Ai::create() - Failed to load ai. Exception: " << e.what() << std::endl;
			ai = std::make_unique<AiPlayer>(); // assign default dummy value in case of loading failure
		}
	}

	return ai;
}
//====

// end of file: Ai.cpp
