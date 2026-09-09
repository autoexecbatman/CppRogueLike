#pragma once

#include <memory>

#include "../Persistent/Persistent.h"

class Creature; // for no circular dependency with Creature.h
struct GameContext; // for dependency injection

enum class AiType
{
	MONSTER = 0,
	CONFUSED_MONSTER = 1,
	// 2 was PLAYER -- removed, PlayerController handles input directly
	SHOPKEEPER = 3,
	MIMIC = 4,
	SPIDER = 5,
	WEB_SPINNER = 6 // append last — integer values are serialized
};

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

	[[nodiscard]] static std::unique_ptr<Ai> create(const json& j);

protected:
	// Protected default constructor - only derived classes can use
	Ai() = default;
};
