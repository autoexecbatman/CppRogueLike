// file: Ai.cpp
#include <memory>
#include <stdexcept>

#include "../Persistent/Persistent.h"
#include "Ai.h"
#include "AiMonster.h"
#include "AiMimic.h"
#include "AiMonsterConfused.h"
#include "AiShopkeeper.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number())
	{
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<AiType>(j["type"].get<int>());
	std::unique_ptr<Ai> ai;

	switch (type)
	{

	case AiType::MONSTER:
	{
		ai = std::make_unique<AiMonster>();
		break;
	}

	case AiType::CONFUSED_MONSTER:
	{
		ai = std::make_unique<AiMonsterConfused>(0, nullptr);
		break;
	}

	case AiType::SHOPKEEPER:
	{
		ai = std::make_unique<AiShopkeeper>();
		break;
	}

	case AiType::MIMIC:
	{
		// Default ctor -- possibleDisguises rebuilt lazily on first update via ContentRegistry.
		ai = std::make_unique<AiMimic>();
		break;
	}

	default:
	{
		throw std::runtime_error("Unknown AiType");
	}

	} // end of switch (type)

	ai->load(j);
	return ai;
}

// end of file: Ai.cpp
