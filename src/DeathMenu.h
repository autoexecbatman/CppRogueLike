#pragma once

#include <string>
#include <vector>

#include "BaseMenu.h"

namespace CppRL
{

	struct GameContext;

	class DeathMenu : public BaseMenu
	{
	private:
		void render(GameContext& ctx) const;
		void handle_input(GameContext& ctx);

		int dungeonLevel{};
		int playerLevel{};
		int playerXp{};
		int killCount{};
		std::string playerClass{};
		std::string playerRace{};
		std::vector<std::string> recentMessages{};

	public:
		DeathMenu(GameContext& ctx);
		~DeathMenu() override = default;
		DeathMenu(const DeathMenu&) = delete;
		DeathMenu& operator=(const DeathMenu&) = delete;
		DeathMenu(DeathMenu&&) = delete;
		DeathMenu& operator=(DeathMenu&&) = delete;

		void menu(GameContext& ctx) override;
	};

} // namespace CppRL
