#pragma once

class Gui;
struct GameContext;

class GameLoopCoordinator
{
private:
	// Pacing for mouse path auto-walk: one step every 0.12 s
	double mousePathStepTime{ 0.0 };

	// Helper methods for game loop phases
	void handle_input_phase(GameContext& ctx);
	void handle_update_phase(GameContext& ctx, Gui& gui);
	void handle_render_phase(GameContext& ctx, Gui& gui);
	void handle_menu_check(GameContext& ctx);
	void draw_hover_tooltip(GameContext& ctx);

public:
	// Core game loop coordination
	void handle_gameloop(GameContext& ctx, Gui& gui, int loopNum);
	void update(GameContext& ctx);

	// Opens the screen a run ends on, for the outcome the game state holds: the death
	// screen after removing the save, or the victory screen, whose closing ends the
	// program. Game::tick calls it once, in the tick the outcome was decided, after
	// clearing the menu stack. Refuses, by assertion, a run that has not ended, and a
	// lost run whose player still has hit points.
	//
	// Example:
	//   ctx.player()->die(ctx);                // the run is lost
	//   coordinator.open_end_of_run_screen(ctx); // -> a DeathMenu on ctx.menus
	//   // with RunOutcome::UNDECIDED: aborts, "the run has not ended"
	void open_end_of_run_screen(GameContext& ctx);

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
};
