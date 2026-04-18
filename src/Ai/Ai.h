#pragma once

#include <memory>

#include "../Persistent/Persistent.h"

class Creature; // for no circular dependency with Creature.h
struct GameContext; // for dependency injection

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

	// Type-safe hostility check - replaces dynamic_cast usage
	[[nodiscard]] virtual bool is_hostile() const { return true; } // Most AI types are hostile by default

	// Trade interface - default no-ops; AiShopkeeper overrides all three
	virtual void initiate_trade(Creature& owner, Creature& player, GameContext& ctx);
	[[nodiscard]] virtual bool is_trade_open() const { return false; }
	virtual void set_trade_open(bool) {}

	[[nodiscard]] static std::unique_ptr<Ai> create(const json& j);

protected:
	// Protected default constructor - only derived classes can use
	Ai() = default;

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
};
