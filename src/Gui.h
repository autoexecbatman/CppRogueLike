#pragma once

#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "Colors.h"
#include "LogMessage.h"

namespace CppRL
{

	struct GameContext;

	class Gui
	{
	private:
		ColorPairId guiMessageColor{ ColorPairId::WHITE_BLACK };
		std::string guiMessage{};
		std::vector<std::vector<LogMessage>> displayMessages;

	protected:
		void render_hp_bar(const GameContext& ctx);
		void render_hunger_status(const GameContext& ctx);

	public:
		bool guiInit{ false };

		void gui_init() noexcept;
		void gui_shutdown() noexcept;
		void gui_update(GameContext& ctx);
		void gui_render(const GameContext& ctx);

		void render_player_status(const GameContext& ctx);

		void gui_print_stats(const GameContext& ctx) noexcept;
		void gui_print_log(const GameContext& ctx);

		void load(const nlohmann::json& savedState);
		void save(nlohmann::json& savedState) const;

		void add_display_message(const std::vector<LogMessage>& message);
		void render_messages() noexcept;

		void set_message(const std::string& msg) { guiMessage = msg; }

		void set_message_color(ColorPairId color) { guiMessageColor = color; }

		const std::string& get_message() const { return guiMessage; }

		ColorPairId get_message_color() const { return guiMessageColor; }
	};

} // namespace CppRL
