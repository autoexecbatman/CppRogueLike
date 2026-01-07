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
    Vector2D vec(5, 10);
    EXPECT_EQ(vec.x, 5);
    EXPECT_EQ(vec.y, 10);
}

TEST_F(Vector2DTest, Equality) {
    Vector2D vec1(3, 4);
    Vector2D vec2(3, 4);
    Vector2D vec3(5, 6);

    EXPECT_EQ(vec1, vec2);
    EXPECT_NE(vec1, vec3);
}

TEST_F(Vector2DTest, Addition) {
    Vector2D vec1(1, 2);
    Vector2D vec2(3, 4);
    Vector2D result = vec1 + vec2;

    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
}

TEST_F(Vector2DTest, Subtraction) {
    Vector2D vec1(5, 7);
    Vector2D vec2(2, 3);
    Vector2D result = vec1 - vec2;

    EXPECT_EQ(result.x, 3);
    EXPECT_EQ(result.y, 4);
}

TEST_F(Vector2DTest, ScalarMultiplication) {
    Vector2D vec(2, 3);
    Vector2D result = vec * 3;

    EXPECT_EQ(result.x, 6);
    EXPECT_EQ(result.y, 9);
}
