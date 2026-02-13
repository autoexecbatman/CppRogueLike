#pragma once

//==CONTROLS==
// the enumeration for the controls of the player
enum class Controls
{
    // the controls for the player movement (arrow key values from InputSystem)
    UP_ARROW = 0x103,
    DOWN_ARROW = 0x102,
    LEFT_ARROW = 0x104,
    RIGHT_ARROW = 0x105,

    // Web-compatible WASD movement
    W_KEY = 'w',
    S_KEY = 's',
    A_KEY = 'a',
    D_KEY = 'd',

    // WASD diagonals
    Q_KEY = 'q',
    E_KEY = 'e',
    Z_KEY = 'z',
    C_KEY = 'c',

    MOUSE = 0x199,

    TEST_COMMAND = 'x',

    WAIT = 'h',
    PICK = 'p',
    DROP = 'l',
    INVENTORY = 'i',
    ESCAPE = 27,
    CHAR_SHEET = '@',
    DESCEND = '>',
    TARGET = 't',
    TOGGLE_GRIP = 'T',
    QUIT = '~',
    DEBUG = 'b',
    ITEM_DISTRIBUTION = 'B',
    REVEAL = 'm',
    REGEN = 'n',
    OPEN_DOOR = 'o',
    CLOSE_DOOR = 'k',
    REST = 'r',
    HELP = '?',
    HIDE = 'H',
    CAST = 'C',
    USE = 'u',
};
