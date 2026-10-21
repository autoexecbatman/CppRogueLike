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
	std::unique_ptr<Ai> ai = make(parse_ai_type(source.at("type").get<std::string>()));
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
		// Its two distances as built; a save's own come back through AiMonsterRanged::load()
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
		// Its disguises come from on_created when it is built, or on its first update after a load.
		ai = std::make_unique<AiMimic>();
		break;
	}

	case AiType::SPIDER:
	{
		// not lying in wait as built; a save's ambush comes back through AiSpider::load()
		ai = std::make_unique<AiSpider>();
		break;
	}

	case AiType::WEB_SPINNER:
	{
		// not lying in wait as built; a save's ambush comes back through AiSpider::load()
		ai = std::make_unique<AiWebSpinner>();
		break;
	}

	case AiType::GIANT_SPIDER:
	{
		// not lying in wait as built; a save's ambush comes back through AiSpider::load()
		ai = std::make_unique<AiGiantSpider>();
		break;
	}

	} // end of switch (type)

	return ai;
}
