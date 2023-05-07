#ifndef CHATGPT_H
#define CHATGPT_H

#include <iostream>
#include <curses.h>
#undef MOUSE_MOVED
#define NOMINMAX
#include <nlohmann/json.hpp>
#include "include/openai/openai.hpp"

class ChatGPT {
public:
    void start_chat();

private:
    void user_input();
    void bot_output_goblin();
    void bot_output_dragon();
    void bot_output_dungeonmaster();
    void bot_output_oldsage();
    void bot_output_healthpotion();

    nlohmann::json conversation_history;
};

#endif // CHATGPT_H
