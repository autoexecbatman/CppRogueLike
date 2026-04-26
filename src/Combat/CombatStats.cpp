#include "CombatStats.h"

void CombatStats::load(const json& j)
{
	dr = j.at("dr").get<int>();
	thaco = j.at("thaco").get<int>();
}

void CombatStats::save(json& j) const
{
	j["dr"] = dr;
	j["thaco"] = thaco;
}
