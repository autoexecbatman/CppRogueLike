#pragma once

#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "Persistent.h"

class Creature; // for no circular dependency with Creature.h
struct GameContext; // for dependency injection

// Which mind a creature carries. A save names it rather than numbering it, so what a
// record means does not depend on the order of this list.
enum class AiType
{
	MONSTER,
	CONFUSED_MONSTER,
	SHOPKEEPER,
	MIMIC,
	SPIDER,
	WEB_SPINNER,
	GIANT_SPIDER,
};

// The name a save carries for this Ai.
//
// Deliberately without a default case: adding an AiType makes this and the parser
// below fail to compile under -Wswitch until both know it.
//
// Example:
//   encode_ai_type(AiType::WEB_SPINNER); // -> "web_spinner"
inline std::string_view encode_ai_type(AiType type)
{
	switch (type)
	{
	case AiType::MONSTER:
	{
		return "monster";
	}
	case AiType::CONFUSED_MONSTER:
	{
		return "confused_monster";
	}
	case AiType::SHOPKEEPER:
	{
		return "shopkeeper";
	}
	case AiType::MIMIC:
	{
		return "mimic";
	}
	case AiType::SPIDER:
	{
		return "spider";
	}
	case AiType::WEB_SPINNER:
	{
		return "web_spinner";
	}
	case AiType::GIANT_SPIDER:
	{
		return "giant_spider";
	}
	}

	return "monster";
}

// The Ai a save names. Throws naming what it read, since a record this build cannot
// place must not become whichever one a number happens to land on.
//
// Example:
//   parse_ai_type("giant_spider"); // -> AiType::GIANT_SPIDER
//   parse_ai_type("wyvern");       // throws std::runtime_error
inline AiType parse_ai_type(std::string_view name)
{
	if (name == "monster")
	{
		return AiType::MONSTER;
	}
	if (name == "confused_monster")
	{
		return AiType::CONFUSED_MONSTER;
	}
	if (name == "shopkeeper")
	{
		return AiType::SHOPKEEPER;
	}
	if (name == "mimic")
	{
		return AiType::MIMIC;
	}
	if (name == "spider")
	{
		return AiType::SPIDER;
	}
	if (name == "web_spinner")
	{
		return AiType::WEB_SPINNER;
	}
	if (name == "giant_spider")
	{
		return AiType::GIANT_SPIDER;
	}

	throw std::runtime_error(std::format("unknown ai type '{}'", name));
}

//==AI==
class Ai : public Persistent
{
public:
	virtual ~Ai() noexcept = default;
	Ai(const Ai&) = delete;
	Ai& operator=(const Ai&) = delete;
	Ai(Ai&&) noexcept = delete;
	Ai& operator=(Ai&&) noexcept = delete;

	virtual void update(Creature& owner, GameContext& ctx) = 0;

	// What this Ai is, which its save writes and Ai::create reads back.
	[[nodiscard]] virtual AiType get_ai_type() const noexcept = 0;

	[[nodiscard]] static std::unique_ptr<Ai> create(const json& j);

protected:
	// Protected default constructor - only derived classes can use
	Ai() = default;
};
