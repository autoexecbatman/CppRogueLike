#pragma once

class Gui;
struct GameContext;

class GameLoopCoordinator
{
public:
	// Core game loop coordination
	void handle_gameloop(GameContext& ctx, Gui& gui, int loopNum);
	void update(GameContext& ctx);

private:
	// Pacing for mouse path auto-walk: one step every 0.12 s
	double mousePathStepTime{ 0.0 };

	// Helper methods for game loop phases
	void handle_initialization(GameContext& ctx);
	void handle_input_phase(GameContext& ctx);
	void handle_update_phase(GameContext& ctx, Gui& gui);
	void handle_render_phase(GameContext& ctx, Gui& gui);
	void handle_menu_check(GameContext& ctx);
	void draw_hover_tooltip(GameContext& ctx);
};
