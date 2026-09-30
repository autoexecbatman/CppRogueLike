#include <memory>
#include <stdexcept>

#include "Ai.h"
#include "AiGiantSpider.h"
#include "AiMimic.h"
#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiShopkeeper.h"
#include "AiSpider.h"
#include "AiWebSpinner.h"
#include "Persistent.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& j)
{
	const AiType type = parse_ai_type(j.at("type").get<std::string>());
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

	case AiType::SPIDER:
	{
		// poisonChance restored from JSON by AiSpider::load()
		ai = std::make_unique<AiSpider>();
		break;
	}

	case AiType::WEB_SPINNER:
	{
		// poisonChance restored from JSON by AiSpider::load()
		ai = std::make_unique<AiWebSpinner>();
		break;
	}

	case AiType::GIANT_SPIDER:
	{
		// poisonChance restored from JSON by AiSpider::load()
		ai = std::make_unique<AiGiantSpider>();
		break;
	}

	} // end of switch (type)

	ai->load(j);
	return ai;
}
