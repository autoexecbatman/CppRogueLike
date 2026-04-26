#pragma once

#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CorpseData
{
private:
	std::string corpseName{};

public:
	explicit CorpseData(std::string_view name)
		: corpseName(name) {}

	[[nodiscard]] const std::string& get_corpse_name() const noexcept { return corpseName; }

	void set_corpse_name(std::string_view name) { corpseName = name; }

	void load(const json& j);
	void save(json& j) const;
};
