// file: Spellcasting.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "SpellRegistry.h"
#include "Spellcasting.h"
#include "TurnSchedule.h"

std::string_view encode_power_frequency(PowerFrequency frequency)
{
	switch (frequency)
	{

	case PowerFrequency::AT_WILL:
	{
		return "at_will";
	}

	case PowerFrequency::ONCE_PER_DAY:
	{
		return "once_per_day";
	}
	}

	std::unreachable();
}

PowerFrequency parse_power_frequency(std::string_view name)
{
	if (name == "at_will")
	{
		return PowerFrequency::AT_WILL;
	}
	if (name == "once_per_day")
	{
		return PowerFrequency::ONCE_PER_DAY;
	}

	throw std::runtime_error(std::format("unknown power frequency '{}'", name));
}

std::string_view encode_caster_class(CasterClass casterClass)
{
	switch (casterClass)
	{

	case CasterClass::WIZARD:
	{
		return "wizard";
	}

	case CasterClass::CLERIC:
	{
		return "cleric";
	}

	case CasterClass::NONE:
	{
		assert(false && "encode_caster_class: no record names a caster class of none");
		return "none";
	}
	}

	std::unreachable();
}

CasterClass parse_caster_class(std::string_view name)
{
	if (name == "wizard")
	{
		return CasterClass::WIZARD;
	}
	if (name == "cleric")
	{
		return CasterClass::CLERIC;
	}

	throw std::runtime_error(std::format("unknown caster class '{}'; a record that casts nothing writes null", name));
}

bool is_power_ready(const InnatePower& power, int now)
{
	return power.frequency == PowerFrequency::AT_WILL || power.readyAgainAt <= now;
}

void record_power_use(InnatePower& power, int now)
{
	if (power.frequency == PowerFrequency::ONCE_PER_DAY)
	{
		power.readyAgainAt = now + ROUNDS_PER_DAY * TIME_UNITS_PER_ROUND;
	}
}

void spend_memorized(Spellcasting& spellcasting, std::string_view key)
{
	const auto copy = std::ranges::find(spellcasting.memorizedSpells, key);
	assert(copy != spellcasting.memorizedSpells.end() && "spend_memorized: a spell was cast that was not memorized");
	spellcasting.memorizedSpells.erase(copy);
}

nlohmann::json encode_spellcasting(const Spellcasting& spellcasting)
{
	nlohmann::json powers = nlohmann::json::array();
	for (const InnatePower& power : spellcasting.innatePowers)
	{
		powers.push_back(nlohmann::json{
			{ "spell", power.spell },
			{ "frequency", encode_power_frequency(power.frequency) },
			{ "readyAgainAt", power.readyAgainAt } });
	}

	const nlohmann::json training = spellcasting.training.has_value()
		? nlohmann::json{
			  { "casterClass", encode_caster_class(spellcasting.training->casterClass) },
			  { "casterLevel", spellcasting.training->casterLevel }
		  }
		: nlohmann::json(nullptr);

	return nlohmann::json{
		{ "memorizedSpells", spellcasting.memorizedSpells },
		{ "training", training },
		{ "innatePowers", powers }
	};
}

Spellcasting parse_spellcasting(const nlohmann::json& record)
{
	Spellcasting spellcasting{};
	spellcasting.memorizedSpells = record.at("memorizedSpells").get<std::vector<std::string>>();

	const nlohmann::json& training = record.at("training");
	if (!training.is_null())
	{
		spellcasting.training = SpellcasterTraining{
			parse_caster_class(training.at("casterClass").get<std::string>()),
			training.at("casterLevel").get<int>()
		};
	}

	for (const nlohmann::json& power : record.at("innatePowers"))
	{
		spellcasting.innatePowers.push_back(InnatePower{
			power.at("spell").get<std::string>(),
			parse_power_frequency(power.at("frequency").get<std::string>()),
			power.at("readyAgainAt").get<int>() });
	}
	return spellcasting;
}

// end of file: Spellcasting.cpp
