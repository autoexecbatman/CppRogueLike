#include "CorpseData.h"

void CorpseData::load(const json& j)
{
	corpseName = j.at("corpseName").get<std::string>();
}

void CorpseData::save(json& j) const
{
	j["corpseName"] = corpseName;
}
