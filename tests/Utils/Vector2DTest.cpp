#include <gtest/gtest.h>
#include "src/Utils/Vector2D.h"

class Vector2DTest : public ::testing::Test {
};

TEST_F(Vector2DTest, DefaultConstructor) {
    Vector2D vec;
    EXPECT_EQ(vec.x, 0);
    EXPECT_EQ(vec.y, 0);
}

TEST_F(Vector2DTest, ParameterizedConstructor) {
    Vector2D vec(5, 10);  // Constructor is (y, x)
    EXPECT_EQ(vec.y, 5);
    EXPECT_EQ(vec.x, 10);
}

TEST_F(Vector2DTest, Equality) {
    Vector2D vec1(3, 4);  // (y, x)
    Vector2D vec2(3, 4);
    Vector2D vec3(5, 6);

    EXPECT_EQ(vec1, vec2);
    EXPECT_NE(vec1, vec3);
}

TEST_F(Vector2DTest, Addition) {
    Vector2D vec1(1, 2);  // y=1, x=2
    Vector2D vec2(3, 4);  // y=3, x=4
    Vector2D result = vec1 + vec2;

    EXPECT_EQ(result.y, 4);  // 1+3
    EXPECT_EQ(result.x, 6);  // 2+4
}

TEST_F(Vector2DTest, Subtraction) {
    Vector2D vec1(5, 7);  // y=5, x=7
    Vector2D vec2(2, 3);  // y=2, x=3
    Vector2D result = vec1 - vec2;

    EXPECT_EQ(result.y, 3);  // 5-2
    EXPECT_EQ(result.x, 4);  // 7-3
}

TEST_F(Vector2DTest, ScalarMultiplication) {
    Vector2D vec(2, 3);  // y=2, x=3
    Vector2D result = vec * 3;

    EXPECT_EQ(result.y, 6);  // 2*3
    EXPECT_EQ(result.x, 9);  // 3*3
}
