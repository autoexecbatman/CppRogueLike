#pragma once

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace CppRL
{

class ExperienceReward
{
private:
	int xp{};

public:
	explicit ExperienceReward(int xpValue) noexcept :
		xp(xpValue)
	{
	}

	[[nodiscard]] int get_xp() const noexcept { return xp; }

	void set_xp(int value) noexcept { xp = value; }

	void add_xp(int amount) noexcept { xp += amount; }

	void load(const nlohmann::json& source);
	void save(nlohmann::json& output) const;
};

} // namespace CppRL
