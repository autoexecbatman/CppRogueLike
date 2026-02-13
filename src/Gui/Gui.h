#pragma once

#include <vector>
#include <string>

#include "../Persistent/Persistent.h"
#include "LogMessage.h"

struct GameContext;

inline constexpr int GUI_HEIGHT{7};
inline int gui_width() { return 118; }

class Gui : public Persistent
{
private:
	int guiMessageColor{0};
	std::string guiMessage{};
	std::vector<std::vector<LogMessage>> displayMessages;

public:
	bool guiInit{false};

	void gui_init() noexcept;
	void gui_shutdown() noexcept;
	void gui_update(GameContext& ctx);
	void gui_render(const GameContext& ctx);

	void render_player_status(const GameContext& ctx);

	void gui_print_stats(const GameContext& ctx) noexcept;
	void gui_print_log(const GameContext& ctx);
	void gui_print_attrs(const GameContext& ctx) noexcept;

	void gui(GameContext& ctx) noexcept;

	void load(const json& j) override;
	void save(json& j) override;

	void add_display_message(const std::vector<LogMessage>& message);
	void render_messages() noexcept;

	void set_message(const std::string& msg) { guiMessage = msg; }
	void set_message_color(int color) { guiMessageColor = color; }
	const std::string& get_message() const { return guiMessage; }
	int get_message_color() const { return guiMessageColor; }

protected:
	void render_hp_bar(const GameContext& ctx);
	void render_hunger_status(const GameContext& ctx);
	void renderMouseLook(const GameContext& ctx);
};
