#include "ExperienceReward.h"

void ExperienceReward::load(const json& j)
{
	xp = j.at("xp").get<int>();
}

void ExperienceReward::save(json& j) const
{
	j["xp"] = xp;
}
