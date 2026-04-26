#include <gtest/gtest.h>
#include <memory>
#include "src/Actor/Destructible.h"

// Pure unit tests for Destructible logic without Game dependencies
// Destructible now only manages HealthPool and ConstitutionTracker

TEST(DestructibleUnitTest, Constructor_InitializesHP)
{
    Destructible d(100);

    EXPECT_EQ(d.get_max_hp(), 100);
    EXPECT_EQ(d.get_hp(), 100);
}

TEST(DestructibleUnitTest, SetHp_AcceptsNegative)
{
    Destructible d(100);
    d.set_hp(-50);
    EXPECT_EQ(d.get_hp(), -50);
}

TEST(DestructibleUnitTest, SetHp_AcceptsOverMax)
{
    Destructible d(100);
    d.set_hp(150);
    EXPECT_EQ(d.get_hp(), 150);
}

TEST(DestructibleUnitTest, IsDead_WhenHpZero)
{
    Destructible d(100);
    d.set_hp(0);
    EXPECT_TRUE(d.is_dead());
}

TEST(DestructibleUnitTest, IsAlive_WhenHpPositive)
{
    Destructible d(100);
    d.set_hp(50);
    EXPECT_FALSE(d.is_dead());
}

TEST(DestructibleUnitTest, Heal_RestoresHP)
{
    Destructible d(100);
    d.set_hp(50);
    int healed = d.heal(20);
    EXPECT_EQ(d.get_hp(), 70);
    EXPECT_EQ(healed, 20);
}

TEST(DestructibleUnitTest, Heal_CapAtMax)
{
    Destructible d(100);
    d.set_hp(90);
    int healed = d.heal(20);
    EXPECT_EQ(d.get_hp(), 100);
    EXPECT_EQ(healed, 10);
}

TEST(DestructibleUnitTest, TempHP_Storage)
{
    Destructible d(100);
    d.set_temp_hp(20);
    EXPECT_EQ(d.get_temp_hp(), 20);
    EXPECT_EQ(d.get_effective_hp(), 120);
}
