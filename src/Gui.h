#pragma once

#include "Colors.h"

#include <string>
#include <vector>

#include "LogMessage.h"
#include "Persistent.h"

struct GameContext;

inline constexpr int GUI_HEIGHT{ 7 };
inline int gui_width()
{
	return 118;
}

class Gui : public Persistent
{
private:
	ColorPairId guiMessageColor{ ColorPairId::WHITE_BLACK };
	std::string guiMessage{};
	std::vector<std::vector<LogMessage>> displayMessages;

public:
	bool guiInit{ false };

	void gui_init() noexcept;
	void gui_shutdown() noexcept;
	void gui_update(GameContext& ctx);
	void gui_render(const GameContext& ctx);

	void render_player_status(const GameContext& ctx);

	void gui_print_stats(const GameContext& ctx) noexcept;
	void gui_print_log(const GameContext& ctx);

	void load(const json& savedState) override;
	void save(json& savedState) override;

	void add_display_message(const std::vector<LogMessage>& message);
	void render_messages() noexcept;

	void set_message(const std::string& msg) { guiMessage = msg; }
	void set_message_color(ColorPairId color) { guiMessageColor = color; }
	const std::string& get_message() const { return guiMessage; }
	ColorPairId get_message_color() const { return guiMessageColor; }

protected:
	void render_hp_bar(const GameContext& ctx);
	void render_hunger_status(const GameContext& ctx);
};
