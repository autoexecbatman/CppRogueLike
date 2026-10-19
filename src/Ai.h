#pragma once

#include <algorithm>
#include <array>
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
	RANGED_MONSTER,
	CONFUSED_MONSTER,
	SHOPKEEPER,
	MIMIC,
	SPIDER,
	WEB_SPINNER,
	GIANT_SPIDER,
};

// The name a save carries for this Ai.
//
// Deliberately without a default case, so an AiType added and left out of this switch,
// or of Ai::create's, does not compile under -Werror=switch and /we4062. The parser is
// a chain of ifs the compiler cannot check, so CodecRoundTripTest's list covers it.
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
	case AiType::RANGED_MONSTER:
	{
		return "ranged_monster";
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

// The Ai a monster record may name. A Confused Ai wraps another for as long as a spell
// lasts and is never authored.
inline constexpr std::array<AiType, 7> AUTHORABLE_AI_TYPES = {
	AiType::MONSTER,
	AiType::RANGED_MONSTER,
	AiType::SHOPKEEPER,
	AiType::MIMIC,
	AiType::SPIDER,
	AiType::WEB_SPINNER,
	AiType::GIANT_SPIDER
};

// Whether a monster record may name this Ai.
//
// Example:
//   is_authorable_ai_type(AiType::MIMIC); // -> true
//   is_authorable_ai_type(AiType::CONFUSED_MONSTER); // -> false
[[nodiscard]] inline bool is_authorable_ai_type(AiType type)
{
	return std::ranges::find(AUTHORABLE_AI_TYPES, type) != AUTHORABLE_AI_TYPES.end();
}

// The next authorable Ai in that order, wrapping at the end; the monster editor's cycle.
//
// Example:
//   next_authorable_ai_type(AiType::MONSTER); // -> AiType::RANGED_MONSTER
//   next_authorable_ai_type(AiType::GIANT_SPIDER); // -> AiType::MONSTER
[[nodiscard]] inline AiType next_authorable_ai_type(AiType type)
{
	const auto found = std::ranges::find(AUTHORABLE_AI_TYPES, type);
	if (found == AUTHORABLE_AI_TYPES.end() || found + 1 == AUTHORABLE_AI_TYPES.end())
	{
		return AUTHORABLE_AI_TYPES.front();
	}
	return *(found + 1);
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
	if (name == "ranged_monster")
	{
		return AiType::RANGED_MONSTER;
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
protected:
	// Protected default constructor - only derived classes can use
	Ai() = default;

public:
	virtual ~Ai() noexcept = default;
	Ai(const Ai&) = delete;
	Ai& operator=(const Ai&) = delete;
	Ai(Ai&&) noexcept = delete;
	Ai& operator=(Ai&&) noexcept = delete;

	virtual void update(Creature& owner, GameContext& ctx) = 0;

	// What this Ai is, which its save writes and Ai::create reads back.
	[[nodiscard]] virtual AiType get_ai_type() const noexcept = 0;

	// What this Ai does once to the creature it was built into, after the creature is
	// otherwise whole. Most Ai do nothing at birth.
	//
	// Example:
	//   ai.on_created(mimic, ctx); // a mimic puts on its first disguise and stops blocking
	virtual void on_created([[maybe_unused]] Creature& owner, [[maybe_unused]] GameContext& ctx) {}

	// A fresh Ai of this kind, as a monster built from its record gets it; nothing is
	// restored into it.
	//
	// Example:
	//   Ai::make(AiType::WEB_SPINNER)->get_ai_type(); // -> AiType::WEB_SPINNER
	[[nodiscard]] static std::unique_ptr<Ai> make(AiType type);

	// The Ai a save describes: made by its kind, then loaded from the record.
	[[nodiscard]] static std::unique_ptr<Ai> create(const json& source);
};
