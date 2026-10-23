#include <memory>

#include "Ai.h"
#include "AiGiantSpider.h"
#include "AiMimic.h"
#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiMonsterRanged.h"
#include "AiShopkeeper.h"
#include "AiSpider.h"
#include "AiWebSpinner.h"
#include "Persistent.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& source)
{
	std::unique_ptr<Ai> ai =
		make(parse_ai_type(source.at("type").get<std::string>()));
	ai->load(source);
	return ai;
}

std::unique_ptr<Ai> Ai::make(AiType type)
{
	std::unique_ptr<Ai> ai;

	switch (type)
	{

	case AiType::MONSTER:
	{
		ai = std::make_unique<AiMonster>();
		break;
	}

	case AiType::RANGED_MONSTER:
	{
		ai = std::make_unique<AiMonsterRanged>();
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
		ai = std::make_unique<AiMimic>();
		break;
	}

	case AiType::SPIDER:
	{
		ai = std::make_unique<AiSpider>();
		break;
	}

	case AiType::WEB_SPINNER:
	{
		ai = std::make_unique<AiWebSpinner>();
		break;
	}

	case AiType::GIANT_SPIDER:
	{
		ai = std::make_unique<AiGiantSpider>();
		break;
	}

	} // end of switch (type)

	return ai;
}
