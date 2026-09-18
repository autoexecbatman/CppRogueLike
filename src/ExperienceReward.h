#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ExperienceReward
{
private:
	int xp{};

public:
	explicit ExperienceReward(int xpValue) noexcept
		: xp(xpValue) {}

	[[nodiscard]] int get_xp() const noexcept { return xp; }

	void set_xp(int value) noexcept { xp = value; }
	void add_xp(int amount) noexcept { xp += amount; }

	void load(const json& j);
	void save(json& j) const;
};
