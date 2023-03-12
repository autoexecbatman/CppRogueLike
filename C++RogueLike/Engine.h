#ifndef PROJECT_PATH_ENGINE_H_
#define PROJECT_PATH_ENGINE_H_

#include <deque>
#include <map>
#include <memory>

#include "Actor.h"
#include "Gui.h"
#include "Literals.h"
#include "Map.h"

class Engine {
public:
    enum class GameStatus : int {
        STARTUP,
        IDLE,
        NEW_TURN,
        VICTORY,
        DEFEAT
    } gameStatus;

    int fovRadius;
    int screenWidth;
    int screenHeight;
    std::shared_ptr<Actor> player;
    std::shared_ptr<Actor> stairs;
    std::unique_ptr<Gui> gui;
    std::unique_ptr<Map> map;
    bool run = true;
    int keyPress = getch();
    int lastKey = getch();
    int level = 0;
    std::vector<std::shared_ptr<Actor>> actors;

    // Constructors and destructor.
    Engine(int screenWidth, int screenHeight);
    ~Engine();

    // Deleted copy constructor and copy assignment operator.
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Deleted move constructor and move assignment operator.
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    // Public member functions.
    void init();
    void update();
    void render();
    void send_to_back(Actor& actor);
    std::shared_ptr<Actor> get_closest_monster(int fromPosX, int fromPosY, double inRange) const;
    bool pick_tile(int* x, int* y, int maxRange);
    void game_menu();
    bool mouse_moved();
    void target();
    void load();
    void save();
    void term();
    void print_container(std::vector<std::shared_ptr<Actor>> actors);
    void key_listener() noexcept { keyPress = getch(); }
    void next_level();
    std::shared_ptr<Actor> get_actor(int x, int y) const;
    void dispay_stats(int level);
    void display_character_sheet();
    int random_number(int min, int max);
    void wizard_eye();

private:
    // Private member variables.
    bool computeFov = false;

    // Private member functions.
};

// Declaration of the global engine object.
extern Engine engine;

#endif // PROJECT_PATH_ENGINE_H_