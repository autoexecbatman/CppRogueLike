#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/ActorTypes/Player.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Attacker.h"
#include "src/Ai/AiPlayer.h"
#include "src/Factories/ItemCreator.h"

using json = nlohmann::json;

// ============================================================================
// PLAYER SERIALIZATION TESTS
// Ensures player state persists correctly including equipment and class/race
// ============================================================================

class PlayerSerializationTest : public ::testing::Test {
protected:
    std::unique_ptr<Player> create_test_player() {
        auto player = std::make_unique<Player>(Vector2D{10, 20});

        // Set up basic stats
        player->set_strength(16);
        player->set_dexterity(14);
        player->set_constitution(15);
        player->set_intelligence(12);
        player->set_wisdom(10);
        player->set_charisma(8);
        player->set_gold(500);

        // Set class/race
        player->playerClassState = Player::PlayerClassState::FIGHTER;
        player->playerRaceState = Player::PlayerRaceState::HUMAN;
        player->playerClass = "Fighter";
        player->playerRace = "Human";
        player->attacksPerRound = 1.5f;
        player->roundCounter = 3;

        // Set up components
        player->destructible = std::make_unique<PlayerDestructible>(30, 2, "player corpse", 0, 18, 10);
        player->attacker = std::make_unique<Attacker>(DamageInfo{1, 8, "1d8"});
        player->ai = std::make_unique<AiPlayer>();

        return player;
    }
};

TEST_F(PlayerSerializationTest, BasicStats_SaveLoad_RoundTrip) {
    auto original = create_test_player();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    EXPECT_EQ(loaded->get_strength(), 16);
    EXPECT_EQ(loaded->get_dexterity(), 14);
    EXPECT_EQ(loaded->get_constitution(), 15);
    EXPECT_EQ(loaded->get_intelligence(), 12);
    EXPECT_EQ(loaded->get_wisdom(), 10);
    EXPECT_EQ(loaded->get_charisma(), 8);
    EXPECT_EQ(loaded->get_gold(), 500);
}

TEST_F(PlayerSerializationTest, ClassAndRace_Preserved) {
    auto original = create_test_player();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    EXPECT_EQ(loaded->playerClassState, Player::PlayerClassState::FIGHTER);
    EXPECT_EQ(loaded->playerRaceState, Player::PlayerRaceState::HUMAN);
    EXPECT_EQ(loaded->playerClass, "Fighter");
    EXPECT_EQ(loaded->playerRace, "Human");
}

TEST_F(PlayerSerializationTest, CombatStats_Preserved) {
    auto original = create_test_player();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    EXPECT_FLOAT_EQ(loaded->attacksPerRound, 1.5f);
    EXPECT_EQ(loaded->roundCounter, 3);
}

TEST_F(PlayerSerializationTest, WebStatus_Preserved) {
    auto original = create_test_player();
    original->webStuckTurns = 5;
    original->webStrength = 10;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    EXPECT_EQ(loaded->webStuckTurns, 5);
    EXPECT_EQ(loaded->webStrength, 10);
}

TEST_F(PlayerSerializationTest, EquippedItems_Preserved) {
    auto original = create_test_player();

    // Create and equip a sword
    auto sword = ItemCreator::create_long_sword(Vector2D{0, 0});
    sword->value = 100;
    original->equippedItems.emplace_back(std::move(sword), EquipmentSlot::RIGHT_HAND);

    // Create and equip armor
    auto armor = ItemCreator::create_chain_mail(Vector2D{0, 0});
    armor->value = 200;
    original->equippedItems.emplace_back(std::move(armor), EquipmentSlot::BODY);

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    ASSERT_EQ(loaded->equippedItems.size(), 2) << "Should have 2 equipped items";

    // Check first item
    EXPECT_EQ(loaded->equippedItems[0].slot, EquipmentSlot::RIGHT_HAND);
    EXPECT_NE(loaded->equippedItems[0].item, nullptr);

    // Check second item
    EXPECT_EQ(loaded->equippedItems[1].slot, EquipmentSlot::BODY);
    EXPECT_NE(loaded->equippedItems[1].item, nullptr);
}

TEST_F(PlayerSerializationTest, NoEquippedItems_HandledGracefully) {
    auto original = create_test_player();
    // Don't add any equipped items

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->equippedItems.empty());
}

TEST_F(PlayerSerializationTest, AllRaces_SaveLoad) {
    std::vector<Player::PlayerRaceState> races = {
        Player::PlayerRaceState::HUMAN,
        Player::PlayerRaceState::ELF,
        Player::PlayerRaceState::DWARF,
        Player::PlayerRaceState::HALFLING,
        Player::PlayerRaceState::GNOME,
        Player::PlayerRaceState::HALFELF
    };

    for (auto race : races) {
        auto player = create_test_player();
        player->playerRaceState = race;

        json j;
        player->save(j);

        auto loaded = std::make_unique<Player>(Vector2D{0, 0});
        loaded->load(j);

        EXPECT_EQ(loaded->playerRaceState, race) << "Race mismatch for " << static_cast<int>(race);
    }
}

TEST_F(PlayerSerializationTest, AllClasses_SaveLoad) {
    std::vector<Player::PlayerClassState> classes = {
        Player::PlayerClassState::FIGHTER,
        Player::PlayerClassState::ROGUE,
        Player::PlayerClassState::CLERIC,
        Player::PlayerClassState::WIZARD
    };

    for (auto playerClass : classes) {
        auto player = create_test_player();
        player->playerClassState = playerClass;

        json j;
        player->save(j);

        auto loaded = std::make_unique<Player>(Vector2D{0, 0});
        loaded->load(j);

        EXPECT_EQ(loaded->playerClassState, playerClass) << "Class mismatch for " << static_cast<int>(playerClass);
    }
}

TEST_F(PlayerSerializationTest, Components_Preserved) {
    auto original = create_test_player();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Player>(Vector2D{0, 0});
    loaded->load(j);

    ASSERT_NE(loaded->destructible, nullptr) << "Destructible not loaded";
    ASSERT_NE(loaded->attacker, nullptr) << "Attacker not loaded";
    ASSERT_NE(loaded->ai, nullptr) << "AI not loaded";

    EXPECT_EQ(loaded->destructible->get_max_hp(), 30);
}
