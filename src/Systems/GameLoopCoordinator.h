// file: Systems/GameLoopCoordinator.h
#ifndef GAME_LOOP_COORDINATOR_H
#define GAME_LOOP_COORDINATOR_H

#pragma once

class Game;
class Gui;

class GameLoopCoordinator 
{
public:
    GameLoopCoordinator() = default;
    ~GameLoopCoordinator() = default;

    // Core game loop coordination
    void handle_gameloop(Game& game, Gui& gui, int loopNum);

private:
    // Helper methods for game loop phases
    void handle_initialization(Game& game);
    void handle_input_phase(Game& game);
    void handle_update_phase(Game& game, Gui& gui);
    void handle_render_phase(Game& game, Gui& gui);
    void handle_menu_check(Game& game);
};

#endif // GAME_LOOP_COORDINATOR_H
// end of file: Systems/GameLoopCoordinator.h
