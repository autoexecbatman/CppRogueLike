#include <gtest/gtest.h>

#include "src/Core/Paths.h"
#include "src/Systems/TileConfig.h"

int main(int argc, char** argv)
{
    TileConfig::instance().load(Paths::TILE_CONFIG);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
