#include "src/Factories/MonsterCreator.h"
#include "src/Core/Paths.h"
#include "src/Factories/ItemCreator.h"
#include <stdexcept>
#include "src/Actor/Creature.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

class MonsterCreatorTest : public ::testing::Test
{
protected:
    MockGameContext mock;
    GameContext ctx{};

    void SetUp() override
    {
        MonsterCreator::load("data/content/monsters.json");
        ItemCreator::load(Paths::ITEMS);
        ctx = mock.to_game_context();
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

// A monster's equipment is authored as an item key and a slot name, and nothing
// makes the pair agree. The same rule the player equips by decides it, and a
// mismatch is an authoring error: it names the monster and the slot rather than
// arming the creature with something it cannot use.
TEST_F(MonsterCreatorTest, EquipmentThatDoesNotFitItsSlotIsRefused)
{
    MonsterParams params = MonsterCreator::get_params("goblin");
    params.equipment.clear();
    params.equipment.push_back({ EquipmentSlot::MISSILE_WEAPON, "dagger" });

    EXPECT_THROW(
        MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx),
        std::runtime_error);
}

// The equipment every monster is actually authored with fits the slot named for
// it, so the rule refuses nothing the game ships.
TEST_F(MonsterCreatorTest, EveryAuthoredMonsterArmsItself)
{
    for (const std::string& key : { "goblin", "orc", "archer", "mage", "ogre", "kobold" })
    {
        const MonsterParams& params = MonsterCreator::get_params(key);
        EXPECT_NO_THROW(MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx)) << key;
    }
}
