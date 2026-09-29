#include <gtest/gtest.h>
#include "src/Vector2D.h"

class Vector2DTest : public ::testing::Test {
};

TEST_F(Vector2DTest, DefaultConstructor) {
    Vector2D vec;
    EXPECT_EQ(vec.x, 0);
    EXPECT_EQ(vec.y, 0);
}

TEST_F(Vector2DTest, ParameterizedConstructor) {
    Vector2D vec(5, 10);  // Constructor is (x, y)
    EXPECT_EQ(vec.x, 5);
    EXPECT_EQ(vec.y, 10);
}

TEST_F(Vector2DTest, Equality) {
    Vector2D vec1(3, 4);  // (y, x)
    Vector2D vec2(3, 4);
    Vector2D vec3(5, 6);

    EXPECT_EQ(vec1, vec2);
    EXPECT_NE(vec1, vec3);
}

TEST_F(Vector2DTest, Addition) {
    Vector2D vec1(1, 2);  // x=1, y=2
    Vector2D vec2(3, 4);  // x=3, y=4
    Vector2D result = vec1 + vec2;

    EXPECT_EQ(result.x, 4);  // 1+3
    EXPECT_EQ(result.y, 6);  // 2+4
}

TEST_F(Vector2DTest, Subtraction) {
    Vector2D vec1(5, 7);  // x=5, y=7
    Vector2D vec2(2, 3);  // x=2, y=3
    Vector2D result = vec1 - vec2;

    EXPECT_EQ(result.x, 3);  // 5-2
    EXPECT_EQ(result.y, 4);  // 7-3
}

TEST_F(Vector2DTest, ScalarMultiplication) {
    Vector2D vec(2, 3);  // x=2, y=3
    Vector2D result = vec * 3;

    EXPECT_EQ(result.x, 6);  // 2*3
    EXPECT_EQ(result.y, 9);  // 3*3
}

// The three distances the type carries, which three callers in src now share.
// Chebyshev counts eight-way steps, where a diagonal costs what a cardinal does;
// Manhattan counts four-way steps. A pure diagonal is what tells them apart, and
// is the case that would hide one being computed as the other.
TEST_F(Vector2DTest, ChebyshevCountsEightWaySteps) {
    const Vector2D origin(0, 0);

    EXPECT_EQ(origin.chebyshev_distance_to(Vector2D(3, 3)), 3) << "a diagonal is one step per cell";
    EXPECT_EQ(origin.chebyshev_distance_to(Vector2D(3, 5)), 5) << "the longer side decides";
    EXPECT_EQ(origin.chebyshev_distance_to(Vector2D(-4, 2)), 4) << "direction does not matter";
    EXPECT_EQ(origin.chebyshev_distance_to(origin), 0);
}

TEST_F(Vector2DTest, ManhattanCountsFourWaySteps) {
    const Vector2D origin(0, 0);

    EXPECT_EQ(origin.manhattan_distance_to(Vector2D(3, 3)), 6) << "a diagonal is two steps per cell";
    EXPECT_EQ(origin.manhattan_distance_to(Vector2D(3, 5)), 8);
    EXPECT_EQ(origin.manhattan_distance_to(Vector2D(-4, 2)), 6);
    EXPECT_EQ(origin.manhattan_distance_to(origin), 0);
}

// The two agree along a row or a column and part company on a diagonal, which is
// the whole reason the game has both.
TEST_F(Vector2DTest, TheTwoAgreeInLineAndPartOnTheDiagonal) {
    const Vector2D origin(0, 0);
    const Vector2D alongARow(6, 0);
    const Vector2D onTheDiagonal(6, 6);

    EXPECT_EQ(origin.chebyshev_distance_to(alongARow), origin.manhattan_distance_to(alongARow));
    EXPECT_LT(origin.chebyshev_distance_to(onTheDiagonal), origin.manhattan_distance_to(onTheDiagonal));
}

TEST_F(Vector2DTest, StraightLineDistanceIsEuclidean) {
    const Vector2D origin(0, 0);

    EXPECT_FLOAT_EQ(origin.distance_to(Vector2D(3, 4)), 5.0f) << "the three-four-five triangle";
    EXPECT_FLOAT_EQ(origin.distance_to(origin), 0.0f);
}
