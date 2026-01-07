#include <gtest/gtest.h>
#include "src/Game.h"

// Provide the global game object that the rest of the code expects
Game game;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
