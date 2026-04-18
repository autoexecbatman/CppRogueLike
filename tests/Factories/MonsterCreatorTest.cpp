#include "src/Factories/MonsterCreator.h"
#include "src/Actor/Creature.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

class MonsterCreatorTest : public ::testing::Test
{
protected:
    MockGameContext mock;

    void SetUp() override
    {
        MonsterCreator::load("data/content/monsters.json");
    }
};

// AD&D 2e invariant: a monster's effective level equals its Hit Dice count.
// This guards the set_creature_level(hpDice.num) line in create_from_params.
TEST_F(MonsterCreatorTest, MonsterLevelEqualsHitDice)
{
    const MonsterParams& params = MonsterCreator::get_params("troll");
    GameContext ctx = mock.to_game_context();
    auto creature = MonsterCreator::create(Vector2D(0, 0), MonsterId::TROLL, ctx);

    ASSERT_NE(creature, nullptr);
    EXPECT_EQ(creature->get_creature_level(), params.hpDice.num);
}

TEST_F(MonsterCreatorTest, WeakMonsterLevelIsAtLeastOne)
{
    const MonsterParams& params = MonsterCreator::get_params("goblin");
    GameContext ctx = mock.to_game_context();
    auto creature = MonsterCreator::create(Vector2D(0, 0), MonsterId::GOBLIN, ctx);

    ASSERT_NE(creature, nullptr);
    EXPECT_EQ(creature->get_creature_level(), params.hpDice.num);
    EXPECT_GE(creature->get_creature_level(), 1);
}
