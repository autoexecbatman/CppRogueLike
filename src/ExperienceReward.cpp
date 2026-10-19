#include "ExperienceReward.h"

void ExperienceReward::load(const json& source)
{
	xp = source.at("xp").get<int>();
}

void ExperienceReward::save(json& output) const
{
	output["xp"] = xp;
}
