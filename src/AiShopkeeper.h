#pragma once

#include <nlohmann/json_fwd.hpp>

#include "Ai.h"

namespace CppRL
{

	class Creature;
	struct GameContext;

	class AiShopkeeper : public Ai
	{
	private:
		void update(Creature& owner, GameContext& ctx) override;

		[[nodiscard]] AiType get_ai_type() const noexcept override
		{
			return AiType::SHOPKEEPER;
		}

		// Opens its shop, of a kind and quality chosen for the current dungeon
		// level, and stocks it.
		//
		// Example:
		//   shopkeeper.ai->on_created(shopkeeper, ctx);
		//   // shopkeeper.shop is now set
		void on_created(Creature& owner, GameContext& ctx) override;

		void load(const nlohmann::json& source) override;
		void save(nlohmann::json& output) const override;
	};

} // namespace CppRL
