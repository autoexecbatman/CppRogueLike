#include <gtest/gtest.h>
#include "src/Actor/Destructible.h"
#include "src/Actor/Actor.h"

// Pure unit tests for Destructible logic without Game dependencies

TEST(DestructibleUnitTest, Constructor_InitializesValues)
{
    Destructible d(100, 5, "corpse", 50, 20, 10);

    EXPECT_EQ(d.get_max_hp(), 100);
    EXPECT_EQ(d.get_dr(), 5);
    EXPECT_EQ(d.get_xp(), 50);
}

TEST(DestructibleUnitTest, SetHp_AcceptsNegative)
{
    Destructible d(100, 0, "corpse", 0, 20, 10);
    d.set_hp(-50);
    EXPECT_EQ(d.get_hp(), -50);
}

TEST(DestructibleUnitTest, SetHp_AcceptsOverMax)
{
    Destructible d(100, 0, "corpse", 0, 20, 10);
    d.set_hp(150);
    EXPECT_EQ(d.get_hp(), 150);
}

TEST(DestructibleUnitTest, IsDead_WhenHpZero)
{
    Destructible d(100, 0, "corpse", 0, 20, 10);
    d.set_hp(0);
    EXPECT_TRUE(d.is_dead());
}

TEST(DestructibleUnitTest, IsAlive_WhenHpPositive)
{
    Destructible d(100, 0, "corpse", 0, 20, 10);
    d.set_hp(50);
    EXPECT_FALSE(d.is_dead());
}
