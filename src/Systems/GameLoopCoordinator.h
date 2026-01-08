// file: Systems/GameLoopCoordinator.h
#ifndef GAME_LOOP_COORDINATOR_H
#define GAME_LOOP_COORDINATOR_H

#pragma once

class Gui;
struct GameContext;

class GameLoopCoordinator
{
public:
    GameLoopCoordinator() = default;
    ~GameLoopCoordinator() = default;

    // Core game loop coordination
    void handle_gameloop(GameContext& ctx, Gui& gui, int loopNum);

private:
    // Helper methods for game loop phases
    void handle_initialization(GameContext& ctx);
    void handle_input_phase(GameContext& ctx);
    void handle_update_phase(GameContext& ctx, Gui& gui);
    void handle_render_phase(GameContext& ctx, Gui& gui);
    void handle_menu_check(GameContext& ctx);
};

#endif // GAME_LOOP_COORDINATOR_H
// end of file: Systems/GameLoopCoordinator.h
