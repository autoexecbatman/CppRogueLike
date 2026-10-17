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
	// bonus, both regenerations, poison, breath and hunger, then the dead are swept. Fed
	// once for every round the clock has finished since the last call, in order, and
	// stopped by the player's death. Apart from the creatures' turns, which is where the
	// AI runs, so a round's upkeep can be driven without a monster deciding anything.
	//
	// Example, after the player spends an action of seven rounds:
	//   apply_round_upkeep(ctx);   // -> rounds 1 to 7 fed; get_rounds_run() is 7
	//   apply_round_upkeep(ctx);   // -> nothing owed, nothing fed
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
