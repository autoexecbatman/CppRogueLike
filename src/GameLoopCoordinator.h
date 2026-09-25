#pragma once

class Gui;
struct GameContext;

class GameLoopCoordinator
{
public:
	// Core game loop coordination
	void handle_gameloop(GameContext& ctx, Gui& gui, int loopNum);
	void update(GameContext& ctx);

	// Everything a passing round does to whoever is standing in it: the Constitution
	// bonus, both regenerations, poison, hunger and curses, then the dead are swept and
	// the clock moves on. Apart from the creatures' turns, which is where the AI runs, so
	// a round's upkeep can be driven without a map or a monster deciding anything.
	//
	// Example:
	//   apply_round_upkeep(ctx);   // -> time is one higher, everyone has felt the round
	void apply_round_upkeep(GameContext& ctx);

private:
	// Pacing for mouse path auto-walk: one step every 0.12 s
	double mousePathStepTime{ 0.0 };

	// Helper methods for game loop phases
	void handle_input_phase(GameContext& ctx);
	void handle_update_phase(GameContext& ctx, Gui& gui);
	void handle_render_phase(GameContext& ctx, Gui& gui);
	void handle_menu_check(GameContext& ctx);
	void draw_hover_tooltip(GameContext& ctx);
};
