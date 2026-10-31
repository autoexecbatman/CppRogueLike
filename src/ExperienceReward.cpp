#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "ExperienceReward.h"

namespace CppRL
{

void ExperienceReward::load(const nlohmann::json& source)
{
	xp = source.at("xp").get<int>();
}

void ExperienceReward::save(nlohmann::json& output) const
{
	output["xp"] = xp;
}

} // namespace CppRL
